# API Client for Google Sheets

## Cliente en C para consumir la Google Sheets API v4 (metadata pública)

### Autor: Ponce Juan

---

## Descripción

Worker (proceso de línea de comandos) escrito en C que consulta la **Google Sheets
API v4**, obtiene la metadata de una spreadsheet pública y la persiste en una base
SQLite siguiendo un paradigma de "second brain": el recurso externo se guarda de
forma polimórfica (JSON en una columna `TEXT`) bajo un esquema fijo de tablas,
permitiendo versionar los cambios de schema del proveedor a lo largo del tiempo.

Uso previsto:

```bash
./sheets_client "https://docs.google.com/spreadsheets/d/1AbCdEfGhIjKlMnOpQrStUvWxYz/edit"
# también acepta el ID pelado:
./sheets_client 1AbCdEfGhIjKlMnOpQrStUvWxYz
```

---

## Contrato con el API externo (Google Sheets API v4)

Este es el "contrato" que el cliente espera/respeta al hablar con Google Sheets. La
versión vigente del API es **v4**. Documentación oficial:
<https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/get>.

### Endpoint

```
GET https://sheets.googleapis.com/v4/spreadsheets/{spreadsheetId}
```

### Autenticación

Mediante **API Key simple** pasada como query param `key` (sin OAuth 2.0 — decisión
tomada en clase porque OAuth es complejo de implementar). La key se genera en
Google Cloud Console habilitando "Google Sheets API".

Sitio oficial: <https://console.cloud.google.com/>.

>1. **Crear (o elegir) un proyecto**:
>Entrá a console.cloud.google.com con tu cuenta de Google. Arriba a la izquierda, al lado del logo, hacé clic en el selector de proyectos y creá uno nuevo (o usá uno que ya tengas).

>2. **Habilitar la Google Sheets API**:
>En el menú lateral: APIs & Services → Library. Buscá "Google Sheets API" y hacé clic en Enable. Sin este paso, la key no va a poder llamar a ese servicio aunque exista.

>3. **Crear la API Key**:
>APIs & Services → Credentials → Create Credentials → API key. Google te genera la key al toque y te la muestra en un cuadro — copiala, la vas a necesitar para el código.

>4. **Restringir la key a Sheets únicamente**:
>Con la key creada, hacé clic en ella para editarla. En "API restrictions" elegí "Restrict key" y marcá solo "Google Sheets API". Así, aunque alguien te robe la key, no puede usarla para otras APIs de Google.

>5. **Restringir por IP con máscara /16**:
>En la misma pantalla de edición, en "Application restrictions" elegí "IP addresses". Ahí agregás el bloque de IPs permitido en notación CIDR, por ejemplo 181.230.0.0/16.

>6. **Calcular tu bloque /16**:
>Buscá "cual es mi ip" en Google para ver tu IP pública actual (ej: 181.230.45.12). Tomá los primeros dos números, poné los últimos dos en 0, y agregale /16 al final → 181.230.0.0/16. Eso cubre 65.536 IPs posibles, así que si tu proveedor te reasigna otra dentro de ese rango, no tenés que tocar nada.

>7. **Guardar y usar la key**:
>Click en Save. La key queda lista para usarse como query param en tus requests: ...?key=TU_API_KEY. Guardala en una variable de entorno o archivo de config, nunca hardcodeada en el repo público.


- **Restricción de IP:** la key se restringe por IP en Cloud Console, usando una
  máscara `/16` (cubre 65.536 direcciones). Esto evita tener que actualizar la
  restricción cada vez que el proveedor de internet reasigna una IP dentro de ese
  mismo bloque. Si la IP asignada cae fuera del bloque `/16` configurado, hay que
  actualizar manualmente la restricción en Cloud Console.
- **Prerrequisito indispensable:** la spreadsheet a consultar debe estar compartida
  como "Cualquiera con el enlace puede ver" (pública). Con API key simple no hay
  identidad de usuario detrás del pedido, así que sin ese permiso público la API
  devuelve 403 aunque la key sea válida.
- Al ser API key simple, **no hay soporte para escritura**: cualquier operación de
  modificación de la spreadsheet requeriría OAuth 2.0 o Service Account, fuera del
  alcance de este proyecto.

### Request

| Parámetro       | Ubicación | Valor                    | Obligatorio |
| --------------- | --------- | ------------------------ | ----------- |
| `spreadsheetId` | path      | ID de la spreadsheet     | Sí          |
| `key`           | query     | `<API_KEY>`               | Sí          |
| `fields`        | query     | acota los campos de la respuesta (ej. `properties,sheets.properties`) | No |
| `includeGridData` | query   | `false` (default) — si fuera `true`, trae también valores de celdas, lo cual queda **fuera de alcance** | No |

**Input del cliente.** El programa recibe una URL de Sheets o un ID pelado. Se
extrae el `spreadsheetId` de cualquiera de estos formatos:

- `https://docs.google.com/spreadsheets/d/<ID>/edit...`
- `<ID>` (string alfanumérico, sin URL)

Ejemplo de request completo:

```
GET https://sheets.googleapis.com/v4/spreadsheets/1AbCdEfGhIjKlMnOpQrStUvWxYz?key=API_KEY
```

### Response esperada (schema)

| Campo             | Tipo   | Descripción                                      |
| ------------------ | ------ | ------------------------------------------------- |
| `spreadsheetId`    | string | ID del archivo                                     |
| `properties`       | object | Metadata general del archivo (ver abajo)           |
| `sheets`           | array  | Lista de pestañas (recurso `Sheet`)                |
| `spreadsheetUrl`   | string | URL canónica de acceso                             |

#### `properties`

| Campo         | Tipo   | Notas                                   |
| ------------- | ------ | ---------------------------------------- |
| `title`       | string | Nombre del archivo                       |
| `locale`      | string | Ej. `es_AR`                              |
| `timeZone`    | string | Ej. `America/Argentina/Buenos_Aires`     |
| `autoRecalc`  | string | `ON_CHANGE` \| `MINUTE` \| `HOUR`        |
| `defaultFormat` | object | Formato de celda por defecto (opcional) |

#### `sheets[].properties`

| Campo           | Tipo   | Notas                                   |
| --------------- | ------ | ---------------------------------------- |
| `sheetId`       | number | ID interno de la pestaña                 |
| `title`         | string | Nombre de la pestaña                     |
| `index`         | number | Posición (orden) entre las pestañas      |
| `sheetType`     | string | `GRID` \| `OBJECT` \| `DATA_SOURCE`      |
| `gridProperties`| object | `rowCount`, `columnCount`                |

### Schema mutable / polimorfismo

El schema del proveedor **no es estable**: la cantidad de elementos en `sheets[]`
varía según cuántas pestañas tenga el archivo, y Google puede agregar campos nuevos
a `properties` (por ejemplo, nuevos `sheetType`) sin previo aviso. Por eso se
persiste como JSON crudo (`meta_payload`) en lugar de columnas rígidas.

Casos a tener en cuenta:

- `defaultFormat` es opcional y puede no venir en todas las respuestas.
- `sheetType` puede tomar valores no documentados si Google agrega nuevos tipos.
- La cantidad de objetos en `sheets[]` cambia si el usuario agrega o borra pestañas.

---

## Mapeo a la base de datos

La respuesta del API se proyecta sobre la tabla `assets`:

| Columna        | Origen                                                          |
| -------------- | ---------------------------------------------------------------- |
| `asset_uri`    | `https://docs.google.com/spreadsheets/d/<spreadsheetId>` (`UNIQUE`) |
| `title`        | `properties.title`                                                |
| `entity`       | `"spreadsheet"` (constante)                                       |
| `provider`     | `"google_sheets"` (constante)                                     |
| `meta_payload` | JSON completo de la respuesta (`properties` + `sheets`)           |
| `created_at`   | Gestionado por la DB                                              |
| `updated_at`   | Gestionado por la DB en cada actualización                        |

### Versionado de schema

- `assets.meta_payload` mantiene **siempre el JSON vigente** del recurso.
- Cuando se vuelve a consultar y el payload cambió, el snapshot previo de la fila se
  guarda en `assets_history.history_payload` antes de mutar, preservando el
  historial de versiones del recurso.

Schema de las tablas:
 
```sql
PRAGMA foreign_keys = ON;

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
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
  
CREATE TABLE IF NOT EXISTS assets_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    asset_id INTEGER NOT NULL,
    history_payload TEXT NOT NULL
        CHECK(json_valid(history_payload)), -- Snapshot JSON completo de la fila antes de mutar
    changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE
);
 
 
CREATE INDEX IF NOT EXISTS idx_assets_history_asset_id
ON assets_history(asset_id);
```
 
Qué se agregó respecto a la versión anterior:
 
- `PRAGMA foreign_keys = ON;` — SQLite no valida foreign keys por defecto, hay que activarlo explícitamente en cada conexión.
- `CHECK(json_valid(...))` en `meta_payload` y `history_payload` — rechaza a nivel de base de datos cualquier intento de guardar un JSON corrupto, en vez de descubrirlo recién al leerlo de vuelta.
- `CHECK(entity = ...)` / `CHECK(provider = ...)` — ambos campos son constantes en este proyecto; el CHECK deja eso documentado en el propio schema.
- Índice sobre `assets_history(asset_id)` — SQLite **no crea automáticamente** un índice sobre columnas de foreign key (a diferencia de `asset_uri`, que al ser `UNIQUE` sí tiene su índice implícito). Sin este índice, buscar "todo el historial de un asset" recorre la tabla entera fila por fila.
---

## Estado / Base de datos

La base SQLite (ej. `redes2.db`) debe crearse con las tablas `assets` y
`assets_history` antes de correr el cliente. La API Key debe generarse y
restringirse por IP (`/16`) en Google Cloud Console antes de la primera ejecución.
La capa de persistencia y el cliente HTTP/TLS en C se implementan en una etapa
posterior a este contrato.