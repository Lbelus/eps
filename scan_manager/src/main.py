import argparse
from plugin.scan_manager import tokenize_docs, process_dir, process_inclusion_set, process_exclusion_set, cds_fc_update_json, json_folder_to_csv
# from fastapi import FastAPI, Request, File, UploadFile, BackgroundTasks, Form, HTTPException
# from fastapi.responses import JSONResponse, FileResponse, Response, StreamingResponse


def main():
    parser = argparse.ArgumentParser(description="Make ocr scan and tokenize docs and improve quality through a round trip strategy")
    parser.add_argument(
        "--mode", 
        choices=["scan", "scan_exclude", "scan_include", "round_trip"], 
        required=True, 
        help="Choose the document parser mode"
    )
    args = parser.parse_args()
    if args.mode == "scan":
        process_dir("./data/input/","./data/output/")
        tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/json") # tokenize the newly created Jsons
        json_folder_to_csv("./data/json/", "./data/csv/merged.csv", "EFTA*.json")
    elif args.mode == "scan_exclude":
        process_exclusion_set("./data/input/","./data/output/", "./data/csv/merged.csv") # jsonify the document absent from the csv file 
        tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/json") # tokenize the newly created Jsons
        json_folder_to_csv("./data/json/", "./data/csv/merged_cmd.csv", "EFTA*.json")
    elif args.mode == "scan_include":
        process_exclusion_set("./data/input/","./data/output/", "./data/csv/merged.csv") # jsonify the document absent from the csv file 
        process_exclusion_set("./data/input/","./data/output/", "./data/csv/merged.csv") # jsonify the document absent from the csv file 
        tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/json") # tokenize the newly created Jsons
        json_folder_to_csv("./data/json/", "./data/csv/merged_cmd.csv", "EFTA*.json")
        tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/json") # tokenize the newly created Jsons
        json_folder_to_csv("./data/json/", "./data/csv/merged_cmd.csv", "EFTA*.json")
    elif args.mode == "round_trip":
        cds_fc_update_json("./data/csv/merged.csv", "./data/json/", "./data/json/") # Update the current jsons with the modified data from the CSV
        process_exclusion_set("./data/input/","./data/output/", "./data/csv/merged.csv") # jsonify the document absent from the csv file 
        tokenize_docs("./config/doc_template.yaml", "./data/output", "./data/json") # tokenize the newly created Jsons
        json_folder_to_csv("./data/json/", "./data/csv/merged_cmd.csv", "EFTA*.json")

if __name__ == "__main__":
    main()


