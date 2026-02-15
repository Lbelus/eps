from core.doc_serializer  import json_converter 
from core.token_finder import token_finder, load_token_config
from core.round_trip import json_folder_to_csv, update_json_from_csv
from pprint import pprint
from pathlib import Path
import re

# process file
def process_dir(input_path, output_path):
    converter = json_converter(Path(input_path), Path(output_path))
    converter.process_docs()

# process file present on a list
def process_inclusion_set(input_path, output_path, valid_files):
    converter = json_converter(Path(input_path), Path(output_path))
    converter.set_valid_files(valid_files)
    converter.process_docs()

# process files not present on a list
def process_exclusion_set(input_path, output_path, valid_files):
    converter = json_converter(Path(input_path), Path(output_path))
    converter.set_valid_files(valid_files, True)
    converter.process_docs()

# process files not present on a list
def tokenize_docs(config_path, input_dir, output_dir):
    token_config = load_token_config(config_path)
    tf = token_finder(input_dir, output_dir, token_config)
    tf.process_docs()
    for doc_type, doc_conf in token_config.items():
        if re.match(doc_conf["match"], "EFTA*.json", re.IGNORECASE):
            if doc_conf["parse_title"]:
                pprint(doc_conf["tokens"])


def cds_fc_to_csv_docs():
    json_folder_to_csv("./updated/", "./json_to_csv_test/merged.csv", "EFTA_*.json")

def cds_fc_update_json(csv, json, output):
    update_json_from_csv(
        csv_file=csv,
        json_dir=json,
        output_dir=output
    )

# process_folder("./input/","/output/")
# process_valid_set("./input/","./output_test/", "./json_to_csv_test/dalist.csv")
# process_exclusion_set("./input/","./output_test/", "./data/json_to_csv_test/merged_cmd.csv")
# tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/updated")

