import json
import sys

CATEGORY_TO_QUERY = {
    # Computer Science
    "cs.LG":    "machine learning",
    "cs.AI":    "artificial intelligence",
    "cs.CL":    "natural language processing",
    "cs.CV":    "computer vision",
    "cs.NE":    "neural networks",
    "cs.IR":    "information retrieval",
    "cs.RO":    "robotics",
    "cs.HC":    "human computer interaction",
    "cs.CR":    "cryptography security",
    "cs.DS":    "data structures algorithms",
    "cs.DC":    "distributed computing",
    "cs.SE":    "software engineering",
    "cs.DB":    "database systems",
    "cs.GR":    "computer graphics",
    "cs.PL":    "programming languages",
    "cs.OS":    "operating systems",
    "cs.AR":    "computer architecture",
    "cs.NI":    "computer networks",
    "cs.SY":    "control systems",
    "cs.GT":    "game theory",
    "cs.CG":    "computational geometry",
    "cs.FL":    "formal languages automata",
    "cs.CC":    "computational complexity",
    "cs.MM":    "multimedia",
    "cs.MS":    "mathematical software",
    # Mathematics
    "math.ST":  "statistics",
    "math.NA":  "numerical analysis",
    "math.OC":  "optimization control",
    "math.PR":  "probability theory",
    "math.CO":  "combinatorics",
    "math.LO":  "mathematical logic",
    "math.NT":  "number theory",
    "math.GR":  "group theory",
    # Statistics
    "stat.ML":  "statistical machine learning",
    "stat.TH":  "statistical theory",
    "stat.ME":  "statistical methodology",
    "stat.AP":  "statistical applications",
    "stat.CO":  "computational statistics",
    # Electrical Engineering
    "eess.SP":  "signal processing",
    "eess.IV":  "image video processing",
    "eess.AS":  "audio speech processing",
    "eess.SY":  "systems control engineering",
    # Physics
    "physics.comp-ph": "computational physics",
    "physics.data-an": "data analysis physics",
    "quant-ph":        "quantum computing",
    # Quantitative Biology / Finance
    "q-bio.NC": "computational neuroscience",
    "q-bio.QM": "quantitative biology methods",
    "q-fin.ST": "financial statistics",
    "q-fin.CP": "computational finance",
}

def get_query(categories: str) -> str:
    for cat in categories.split():
        if cat in CATEGORY_TO_QUERY:
            return CATEGORY_TO_QUERY[cat]
    return None

def parse(input_path: str, output_path: str, limit: int = 0):
    written = 0
    skipped = 0

    with open(input_path, "r", encoding="utf-8") as fin, \
         open(output_path, "w", encoding="utf-8") as fout:

        for i, line in enumerate(fin):
            if i % 100_000 == 0:
                print(f"  processed {i:,} lines, written {written:,} pairs...", flush=True)

            line = line.strip()
            if not line:
                continue

            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                skipped += 1
                continue

            title    = (obj.get("title")    or "").replace("\n", " ").strip()
            abstract = (obj.get("abstract") or "").replace("\n", " ").strip()
            categories = obj.get("categories") or ""

            if not title or not abstract:
                skipped += 1
                continue

            query = get_query(categories)
            if query is None:
                skipped += 1
                continue

            fout.write(f"{query}\t{title}\t{abstract}\n")
            written += 1

            if limit and written >= limit:
                break

    print(f"\nDone. Written: {written:,}  Skipped: {skipped:,}")
    print(f"Output: {output_path}")

if __name__ == "__main__":
    input_path  = sys.argv[1] if len(sys.argv) > 1 else "arxiv-metadata-oai-snapshot.json"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "D:\dataset\dataset.tsv"
    limit       = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    print(f"Input:  {input_path}")
    print(f"Output: {output_path}")
    print(f"Limit:  {'all' if not limit else limit}\n")

    parse(input_path, output_path, limit)
