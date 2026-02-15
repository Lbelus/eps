import re
import json
import os
import csv
from pathlib import Path
from pdf2image import convert_from_path
import pytesseract
from bs4 import BeautifulSoup
from PIL import Image, ImageOps, ImageEnhance
from pathlib import Path
from collections import defaultdict

"""
PDF/HTML → JSON converter with OCR.

This module converts:
  • PDFs → images → OCR text → JSON (one entry per page)
  • .htm/.html files → extracted text → JSON (single page)

Typical usage:
    from pathlib import Path
    converter = json_converter(Path("./input"), Path("./output"))
    converter.process_docs()
"""


class json_converter():
    def __init__(self, input_dir, output_dir):
        self.pattern = None
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.valid_set = None
        self.exclusion_list = False

    def set_valid_files(self, csv_file, exclusion_list=False):
        self.valid_set = set()
        self.exclusion_list = exclusion_list
        with open(csv_file, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f, delimiter=";")
            for row in reader:
                self.valid_set.add(row["filename"]) 

    ################ create_json_doc #################
    # Creates a fresh JSON document skeleton.
    # @return dict with filename, pages[], metadata.total_pages
    def create_json_doc(self):
        return {
            "filename": '',
            "pages": [],
            "metadata": {
                "total_pages": ''
            }
        }

    # Convert to grayscale
    # Invert image to make black text on white
    # Enhance contrast
    # L = luminance (grayscale)
    # You can tune the enhance factor
    def preprocess_image(self, image):
        image = image.convert("L") 
        inverted = ImageOps.invert(image)
        enhancer = ImageEnhance.Contrast(inverted)
        enhanced = enhancer.enhance(2.0)
        return enhanced
    ################ convert_pdf_to_json #################
    # Converts a list of PDF pages (images) into a JSON doc via OCR.
    # @param filename: Original filename (string)
    # @param json_doc: JSON structure to fill
    # @param pages: List of PIL images from the PDF
    # @return The updated json_doc
    def convert_pdf_to_json(self, filename, json_doc, pages):
        json_doc["filename"] = filename
        json_doc["metadata"]["total_pages"] = len(pages)
        for index, page in enumerate(pages, start=1):
            processed = self.preprocess_image(page)
            text = pytesseract.image_to_string(processed)
            # text = pytesseract.image_to_string(page)
            json_doc["pages"].append({
                "number": index,
                "text": text
            })
        return json_doc

    ################ convert_htm_to_json #################
    # Parses an .htm/.html file and stores its text into JSON (single page).
    # @param filename: Original filename (string)
    # @param json_doc: JSON structure to fill
    # @param htm_path: Path to the .htm/.html file
    # @return The updated json_doc
    def convert_htm_to_json(self, filename, json_doc, htm_path):
        json_doc["filename"] = filename
        json_doc["metadata"]["total_pages"] = 1
        with open(htm_path, 'r', encoding='utf-8') as file:
            soup = BeautifulSoup(file, 'html.parser')
        text = soup.get_text(separator='\n')
        json_doc["pages"].append({
            "number": 1,
            "text": text
        })
        return json_doc 

    ################ create_file #################
    # Writes the JSON document to disk next to output_dir/filename.json.
    # @param filepath: Original file path (used to derive .json name)
    # @param output_dir: Output directory (Path)
    # @param json_doc: JSON data to write
    # @return None
    def create_file(self, filepath: Path, output_dir: Path, json_doc):
        json_filename = filepath.with_suffix('.json').name
        output_path = str(output_dir) + '/' + json_filename
        with open(output_path, 'w', encoding='utf-8') as file:
            json.dump(json_doc, file, indent=2, ensure_ascii=False)
    
    ################ jsonify #################
    # Dispatches by suffix: PDFs are OCR'd; .htm/.html are parsed; others ignored.
    # @param filepath: Path to the input file
    # @param output_dir: Directory where .json will be saved
    # @return None
    def jsonify(self, filepath: Path, output_dir: Path):
        filepath = Path(filepath)
        json_doc = self.create_json_doc()
        if filepath.suffix.lower() == ".pdf":
            print(f"processing: {filepath.name}")
            pages = convert_from_path(filepath)
            json_doc = self.convert_pdf_to_json(filepath.name, json_doc, pages)
        elif filepath.suffix.lower() == ".htm":
            print(f"processing: {filepath.name}")
            json_doc = self.convert_htm_to_json(filepath.name, json_doc, filepath)
        else:
            print(f"File is not supported: {filepath.name}")
            return
        self.create_file(filepath, output_dir, json_doc)
    
    ################ process_docs #################
    # Processes all supported files from input_dir and writes JSON to output_dir.
    # @return None
    def process_docs(self):
        self.output_dir.mkdir(exist_ok=True, parents=True)
        for filename in self.input_dir.glob("*"):
            if self.exclusion_list is False:
                if self.valid_set is not None and filename.name not in self.valid_set:    
                    continue
            elif self.exclusion_list  is True:
                if self.valid_set is not None and filename.name in self.valid_set:    
                    continue
            if filename.suffix.lower() in [".pdf", ".htm", ".html"]:
                self.jsonify(filename, self.output_dir)

# Example debug calls
def process_folder(input_path, output_path):
    converter = json_converter(Path(input_path), Path(output_path))
    converter.process_docs()

def process_valid_set(input_path, output_path, valid_files):
    converter = json_converter(Path(input_path), Path(output_path))
    converter.set_valid_files(valid_files)
    converter.process_docs()


# process_folder("./input/","/output/")

# process_valid_set("./input/","./output_test/", "./json_to_csv_test/dalist.csv")

