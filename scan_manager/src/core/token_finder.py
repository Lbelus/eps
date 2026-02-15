import re
import json
import yaml
from pathlib import Path
import unicodedata
from rapidfuzz import fuzz, process

################ load_token_config #################
# Load YAML config and return the "documents" section.
# @param path: Path to YAML configuration file
# @return dict mapping doc types to {match, tokens}
def load_token_config(path: str):
    with open(path, 'r', encoding='utf-8') as file:
        config = yaml.safe_load(file)
    return config['documents']

"""
Enrich OCR JSON files with token match positions and extracted values.

- Selects tokens based on filename regex from YAML config.
- Normalizes text (removes diacritics, collapses whitespace).
- Supports:
    * KMP substring search (fast, exact, case-insensitive via casefold)
    * Regex search (powerful patterns, IGNORECASE flag)
"""
class token_finder:

    def __init__(self, input_dir, output_dir, token_config):
        self.pattern = None
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.token_config = token_config
        self.handle_title = False
    
    ################ get_tokens_for_file #################
    # Choose the token list for a file based on its name.
    # @param filename: The JSON filename
    # @return list of token entries (possibly empty)
    def get_tokens_for_file(self, filename: str):
        for doc_type, doc_conf in self.token_config.items():
            if re.match(doc_conf["match"], filename, re.IGNORECASE):
                if doc_conf["parse_title"]:
                    self.handle_title = True
                return doc_conf["tokens"]
        return []
    ################ normalize_text #################
    # Normalize: remove accents, collapse whitespace.
    # @param text: Input text
    # @return normalized text
    def normalize_text(self, text):
        text = unicodedata.normalize("NFKD", text)
        text = ''.join(c for c in text if not unicodedata.combining(c))
        return ' '.join(text.split())
    
    def is_valid_amount(self, val):
        if not re.match(r"^\d[\d\s]*[.,]\d{2}$", val):
            return False
        val = val.replace(" ", "").replace(",", ".")
        try:
            amount = float(val)
        except ValueError:
            return False
        return 480.0 <= amount <= 100_000.0
    ################ longest_prefix_suffix #################
    # Build LPS array for KMP.
    # @param pattern: Pattern string
    # @return List[int] LPS values
    def longest_prefix_suffix(self, pattern):
        size = len(pattern) 
        lps = [None] * size
        lps[0]  = 0
        index   = 1
        length  = 0
        while index < size:
            if pattern[index] == pattern[length]:
                length += 1
                lps[index] = length
                index += 1
            else:
                if length  != 0:
                    length = lps[length - 1]
                else:
                    lps[index] = 0
                    index += 1
        return lps
    
    ################ kmp #################
    # Find all occurrences of 'pattern' in 'text' using KMP (Knuth-Morris-Pratt).
    # https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm
    # @param text: Source text
    # @param pattern: Pattern to search
    # @return List[int] starting indices
    def kmp(self, text, pattern):
        matches = []
        pattern_len = len(pattern)
        text_len = len(text)
        if pattern_len == 0 or text_len < pattern_len:
            return matches
        lps = self.longest_prefix_suffix(pattern)
        index = 0
        jndex = 0 
        while index < text_len:
            if text[index] == pattern[jndex]:
                index += 1
                jndex += 1
                if jndex == pattern_len:
                    matches.append(index - jndex)
                    jndex = lps[jndex - 1]
            elif jndex > 0:
                jndex = lps[jndex - 1]
            else:
                index += 1
        return matches

    ################ fuzzy_find #################
    # Try fuzzy matching the token in the text.
    # Returns list of approximate match positions.
    # @param text: The full normalized page text
    # @param pattern: The token string (normalized)
    # @param threshold: Minimum match score (0-100)
    # @return List[int] start positions
    def fuzzy_find(self, text, pattern, threshold=85):
        window_size = len(pattern)
        matches = []
        for i in range(0, len(text) - window_size + 1):
            window = text[i:i + window_size + 5]  # allow small variation
            score = fuzz.partial_ratio(pattern, window)
            if score >= threshold:
                matches.append(i)
        return matches

    ################ set_tokens #################
    # Extract a fixed-length slice after each token match.
    # @param matches: List of start indices for the token
    # @param token: The token pattern string (normalized)
    # @param text: The normalized text
    # @param offset: Number of characters to capture after the token
    # @return List[str] extracted substrings
    def set_tokens(self, matches, token, text, offset):
        result = []
        token_len = len(token)
        for match in matches:
            pos = match + token_len
            result.append(text[pos+1:pos + offset])
        return result
    

    ################ find_tokens_pos #################
    # For a given page, find matches and values for each configured token.
    # Stores results on the PAGE (page["pos"], page["values"]) to avoid
    # clobbering document-level metadata across pages.
    # @param page: Dict for a single page from the JSON
    # @param token_list: List of token configs for this file
    # @return None
    def find_tokens_pos(self, page, json_doc, token_list):
        raw_text = page["text"]
        text = self.normalize_text(raw_text)
        page["text"] = text
        json_doc.setdefault("metadata", {})
        json_doc["metadata"].setdefault("pos", {})
        json_doc["metadata"].setdefault("values", {})
        print("here")
        for token in token_list:
            print(f"working on token {token}")
            label = token["label"]
            method = token["method"]
            # pattern = self.normalize_text(token["pattern"])
            patterns = token.get("patterns")
            if not patterns:
                patterns = [token["pattern"]]
            if method == "kmp":
                matches = []
                values = []
                for pattern in patterns:
                    print(f"looking for kmp pattern: {pattern}")
                    pattern = self.normalize_text(pattern)
                    matches = self.kmp(text, pattern)
                    values = self.set_tokens(matches, pattern, text, token.get("extract_after", 10))
                    if values:
                        break
#             if method == "kmp":
#                 matches = self.kmp(text, patterns)
#                 values = self.set_tokens(matches, patterns, text, token.get("extract_after", 10))
            elif method == "regex":
                matches = []
                values = []
                for idx, raw_pattern in enumerate(patterns):
                    pattern = self.normalize_text(raw_pattern)
                    matches = []
                    values = []
                    for match in re.finditer(pattern, text, re.IGNORECASE):
                        matches.append(match.start())
                        value = match.group(token.get("group", 0))
                        if token.get("strip_spaces"):
                            value = value.replace(" ", "")
                        if token.get("valid_amount") and not self.is_valid_amount(value):
                           value = "" 
                        if value.strip():
                            matches.append(match.start())
                            values.append(value)
                             

                    if values:  # Only keep first match set
                        break
#                 matches = []
#                 values = []
#                 for match in re.finditer(pattern, text, re.IGNORECASE):
#                     matches.append(match.start())
#                     value = match.group(token.get("group", 0))
#                     if token.get("strip_spaces"):
#                         value = value.replace(" ", "")
#                     values.append(value)
            elif method == "fuzzy":
                threshold = token.get("threshold", 85)
                matches = []
                values = []
                for pattern in patterns:
                    pattern = self.normalize_text(pattern)
                    matches = self.fuzzy_find(text, pattern, threshold)
                    values = self.set_tokens(matches, pattern, text, token.get("extract_after", 10))
                    if values:
                        break
#             elif method == "fuzzy":
#                 threshold = token.get("threshold", 85)
#                 matches = self.fuzzy_find(text, patterns, threshold)
#                 values = self.set_tokens(matches, patterns, text, token.get("extract_after", 10))
            json_doc["metadata"]["pos"].setdefault(label, []).extend(matches)
            if token.get("merge") and len(values) > 1:
                merged_value = " ".join(values)
                json_doc["metadata"]["values"].setdefault(label, []).append(merged_value)
            else:
                json_doc["metadata"]["values"].setdefault(label, []).extend(values)
            #json_doc["metadata"]["values"].setdefault(label, []).extend(values)

    ################ update_file #################
    # Write updated JSON to output_dir using same filename.
    # @param filepath: Path to source .json
    # @param output_dir: Output directory
    # @param json_doc: Updated JSON content
    # @return None
    def update_file(self, filepath, output_dir, json_doc):
        json_filename = filepath.with_suffix('.json').name
        output_path = str(output_dir) + '/' + json_filename
        with open(output_path, 'w', encoding='utf-8') as file:
            json.dump(json_doc, file, indent=2, ensure_ascii=False)

    def parse_title(self, json_doc, delimiter): 
        json_doc["metadata"]["title"] = {}
        data =  json_doc["filename"].split(".")
        json_doc["metadata"]["file_extension"] = data[1]
        json_doc["metadata"]["title"] = data[0].split(delimiter)

    ################ process_docs #################
    # Process all *.json files in input_dir and write enriched versions to output_dir.
    # @return None
    def process_docs(self):
        self.output_dir.mkdir(exist_ok=True, parents=True)
        for file_path in self.input_dir.glob("*.json"):
            print(f"processing: {file_path}")
            with open(file_path, 'r', encoding='utf-8') as file:
                json_doc = json.load(file)
                print(f"filename : {file_path.name}")
            token_list = self.get_tokens_for_file(file_path.name)
            if self.handle_title:
                self.parse_title(json_doc, "_")
            for page in json_doc.get("pages", []): 
                print(f"tokenlist {token_list}")
                self.find_tokens_pos(page, json_doc, token_list)
            self.update_file(file_path, self.output_dir, json_doc)

# # Example debug calls
# token_config = load_token_config("./config/doc_template.yaml")
# tf = token_finder("./output", "./updated", token_config)
# tf.process_docs()
# for doc_type, doc_conf in token_config.items():
#     if re.match(doc_conf["match"], "pattern_XXX.json", re.IGNORECASE):


