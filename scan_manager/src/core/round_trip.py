import json
import csv
import re
from pathlib import Path
from collections import defaultdict

def normalize_number(val: str):
    """Convert European decimal comma to dot if it's a number."""
    if isinstance(val, str):
        if re.fullmatch(r"\d{1,3}(?:\d{3})*(?:,\d+)?|\d+(?:,\d+)?", val):
            return val.replace(",", ".")
    return val

def json_folder_to_csv(input_dir, output_csv, pattern="*.json"):
    """
    Convert JSON files in a folder matching a pattern into a merged CSV.
    pattern examples: 'BDL_*.json', 'PVA_*.json', 'CMD_*.json', '*.json'
    """
    rows = []
    all_keys = set()

    # Pick only files matching the pattern
    for file in Path(input_dir).glob(pattern):
        print(f"working on {file}")
        with open(file, "r", encoding="utf-8") as f:
            try:
                data = json.load(f)
            except json.JSONDecodeError:
                print(f"⚠️ Skipping invalid JSON: {file}")
                continue
        meta = data.get("metadata", {})
        title_cols = {f"title_{i}": t for i, t in enumerate(meta.get("title", []))}
        base = {
            "filename": data.get("filename", file.name),
            **title_cols,
            "file_extension": meta.get("file_extension", "")
        }
        values = meta.get("values", {})
        max_len = max((len(v) for v in values.values() if isinstance(v, list)), default=1)
        for i in range(max_len):
            row = base.copy()
            for k, v in values.items():
                if isinstance(v, list):
                    if len(v) == 0:
                        row[k] = ""
                    elif i < len(v):
                        row[k] = normalize_number(v[i])
                    else:
                        row[k] = normalize_number(v[-1])
                else:
                    row[k] = normalize_number(v)
            rows.append(row)
            all_keys.update(row.keys())
    if not rows:
        print(f"⚠️ No JSON files found matching {pattern} in {input_dir}")
        return
    all_keys = ["filename"] + sorted([k for k in all_keys if k != "filename"])
    with open(output_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=all_keys)
        writer.writeheader()
        writer.writerows(rows)

def update_json_from_csv(csv_file, json_dir, output_dir):
    """
    Update JSON files in json_dir using corrected values from csv_file.
    Writes updated JSON into output_dir.
    """
    corrections = defaultdict(list)
    with open(csv_file, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f, delimiter=";")
        for row in reader:
            corrections[row["filename"]].append(row)

    Path(output_dir).mkdir(parents=True, exist_ok=True)
    for filename, rows in corrections.items():
        json_path = Path(json_dir) / filename
        if not json_path.exists():
            print(f"⚠️ JSON file {filename} not found, skipping")
            continue
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        meta = data.setdefault("metadata", {})
        values = meta.setdefault("values", {})
        for key in rows[0].keys():
            if key in ("filename", "file_extension") or key.startswith("title_"):
                continue
            vals = [r[key] for r in rows if r[key] != ""]
            unique_vals = []
            for v in vals:
                if v not in unique_vals:
                    unique_vals.append(v)
            if unique_vals:
                values[key] = unique_vals
        out_path = Path(output_dir) / filename
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

        print(f"Updated {filename} → {out_path}")


# # debug call example
# update_json_from_csv(
#     csv_file="./json_to_csv_test/merged.csv",
#     json_dir="./updated/",
#     output_dir="./json_corrected"
# )

