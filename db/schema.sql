-- Schema de la base SQLite del cliente de Google Sheets.
-- Todas las sentencias usan IF NOT EXISTS: puede ejecutarse más de una vez.

PRAGMA foreign_keys = ON;

------------------------------------------------------
-- ASSETS: estado vigente de cada spreadsheet
------------------------------------------------------

CREATE TABLE IF NOT EXISTS assets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    asset_uri TEXT UNIQUE NOT NULL,
    title TEXT,
    entity TEXT NOT NULL
        CHECK(entity = 'spreadsheet'),
    provider TEXT NOT NULL
        CHECK(provider = 'google_sheets'),
    meta_payload TEXT NOT NULL
        CHECK(json_valid(meta_payload)), -- JSON polimórfico con la metadata del proveedor
    schema_hash TEXT NOT NULL
        CHECK(length(schema_hash) = 64), -- SHA-256 (hex) de la estructura del payload vigente
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

------------------------------------------------------
-- ASSETS_HISTORY: versiones anteriores ante cambios de estructura
------------------------------------------------------

CREATE TABLE IF NOT EXISTS assets_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    asset_id INTEGER NOT NULL,
    history_payload TEXT NOT NULL
        CHECK(json_valid(history_payload)), -- Copia del meta_payload anterior, antes de mutar
    schema_hash TEXT NOT NULL
        CHECK(length(schema_hash) = 64), -- Hash de la estructura que tenía ese snapshot
    changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE
);

------------------------------------------------------
-- ÍNDICES
------------------------------------------------------

CREATE INDEX IF NOT EXISTS idx_assets_history_asset_id
ON assets_history(asset_id);