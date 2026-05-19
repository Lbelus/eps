-- Schema for the EPS REST API court documents store.
-- Mirrors the SQLite layout in scan_manager/data/epstein.db so that
-- scan_manager/src/utils/migration_script.py can copy rows 1:1.
--
-- Apply with:
--   mysql -h 127.0.0.1 -P 3306 -u dev_admin -p test_rest_DB < rest_api/db/schema.sql

CREATE TABLE IF NOT EXISTS documents (
    document_id INT AUTO_INCREMENT PRIMARY KEY,
    filename    VARCHAR(512) NOT NULL UNIQUE,
    source      VARCHAR(255) NOT NULL,
    page_count  INT NOT NULL,
    full_text   LONGTEXT NOT NULL,
    created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FULLTEXT KEY ft_documents_full_text (full_text),
    INDEX idx_documents_source (source)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS pages (
    page_id      INT AUTO_INCREMENT PRIMARY KEY,
    document_id  INT NOT NULL,
    page_number  INT NOT NULL,
    page_text    LONGTEXT NOT NULL,
    INDEX idx_pages_document (document_id),
    UNIQUE KEY uq_pages_document_page (document_id, page_number),
    CONSTRAINT fk_pages_document
        FOREIGN KEY (document_id)
        REFERENCES documents(document_id)
        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
