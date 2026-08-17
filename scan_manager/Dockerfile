# RAG chat API (FastAPI/SSE over the FTS5 + semantic index).
# Runs the conversational RAG server against a local SQLite corpus, talking to
# an OpenAI-compatible model server (Ollama by default) over the network.
FROM python:3.12-slim

WORKDIR /app

# gzip: decompress the tracked epstein.db.gz snapshot at first start.
# curl: container HEALTHCHECK against /rag/health.
RUN apt-get update \
    && apt-get install -y --no-install-recommends gzip curl \
    && rm -rf /var/lib/apt/lists/*

COPY requirements-rag.txt .
RUN pip install --no-cache-dir -r requirements-rag.txt

COPY src ./src
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Default to the free/open-source Ollama backend; compose overrides the URLs to
# reach the ollama service by name. RAG_DB_PATH keeps the DB inside the volume.
ENV RAG_PROVIDER=openai \
    RAG_MODEL=qwen3:4b \
    RAG_BASE_URL=http://ollama:11434/v1 \
    RAG_EMBED_URL=http://ollama:11434 \
    RAG_DB_PATH=/app/data/epstein.db \
    PYTHONUNBUFFERED=1

EXPOSE 8000

HEALTHCHECK --interval=30s --timeout=5s --start-period=40s --retries=3 \
    CMD curl -fsS http://localhost:8000/rag/health || exit 1

ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["uvicorn", "--app-dir", "src", "api.rag_server:app", "--host", "0.0.0.0", "--port", "8000"]
