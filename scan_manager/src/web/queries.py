from itertools import product

BASE_TERMS = [
    '"Epstein files"',
    '"Jeffrey Epstein" files',
    '"Epstein documents"',
    '"Epstein records"',
    '"Epstein release"',
    '"Epstein court records"',
    '"Epstein flight logs"',
    '"Epstein black book"',
    '"Epstein deposition"',
]

DOC_TYPES = [
    "pdf",
    '"court filing"',
    '"grand jury"',
    '"deposition"',
    '"transcript"',
    '"affidavit"',
    '"exhibit"',
    '"sealed"',
    '"unsealed"',
    '"FOIA"',
    '"metadata"',
    '"email"',
    '"contact list"',
    '"flight manifest"',
]

PEOPLE = [
    "Ghislaine Maxwell",
    "Jean-Luc Brunel",
    "Les Wexner",
    "Alan Dershowitz",
    "Virginia Giuffre",
    "Sarah Ransome",
    "Prince Andrew",
    "Bill Clinton",
    "Donald Trump",
]

PLACES = [
    '"Little Saint James"',
    '"Palm Beach"',
    '"New York"',
    '"New Mexico"',
    '"Paris"',
    '"US Virgin Islands"',
]

EVENTS = [
    '"sex trafficking"',
    '"recruitment"',
    '"massage"',
    '"settlement"',
    '"lawsuit"',
    '"indictment"',
    '"plea deal"',
    '"victim"',
    '"witness"',
]

DATES = [
    "2002", "2005", "2006", "2007", "2008",
    "2015", "2019", "2020", "2023", "2024", "2025", "2026"
]

SITES = [
    "site:justice.gov",
    "site:fbi.gov",
    "site:archives.gov",
    "site:documentcloud.org",
    "site:courtlistener.com",
    "site:nycourts.gov",
    "site:govinfo.gov",
]

OPERATORS = [
    "",
    "filetype:pdf",
    "intitle:Epstein",
    "inurl:epstein",
]

def generate_queries():
    seen = set()

    groups = [
        (BASE_TERMS, DOC_TYPES),
        (BASE_TERMS, PEOPLE),
        (BASE_TERMS, PLACES),
        (BASE_TERMS, EVENTS),
        (BASE_TERMS, DATES),
        (BASE_TERMS, SITES),
        (BASE_TERMS, DOC_TYPES, PEOPLE),
        (BASE_TERMS, DOC_TYPES, PLACES),
        (BASE_TERMS, DOC_TYPES, EVENTS),
        (BASE_TERMS, PEOPLE, DATES),
        (BASE_TERMS, PLACES, DATES),
        (BASE_TERMS, EVENTS, DATES),
        (BASE_TERMS, DOC_TYPES, PEOPLE, DATES),
        (BASE_TERMS, DOC_TYPES, PLACES, DATES),
        (BASE_TERMS, DOC_TYPES, EVENTS, DATES),
        (BASE_TERMS, DOC_TYPES, PEOPLE, PLACES),
        (BASE_TERMS, DOC_TYPES, PEOPLE, EVENTS),
        (BASE_TERMS, DOC_TYPES, PEOPLE, PLACES, DATES),
    ]

    for group in groups:
        for combo in product(*group):
            for op in OPERATORS:
                q = " ".join([part for part in (*combo, op) if part]).strip()
                if q not in seen:
                    seen.add(q)
                    yield q

if __name__ == "__main__":
    for i, query in enumerate(generate_queries(), 1):
        print(f"{i}: {query}")
