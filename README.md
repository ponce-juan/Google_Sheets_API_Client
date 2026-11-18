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

## Dependencias

Aparte de la glibc, el proyecto usa:

| Librería | Para qué se usa |
|---|---|
| **OpenSSL** (`libssl`, `libcrypto`) | TLS sobre el socket TCP y SHA-256 del `schema_hash` |
| **SQLite3** (`libsqlite3`) | Persistencia en un archivo `.db` |

No usa ninguna otra librería externa. Quedan dentro de la glibc o son código propio:

- **Sockets TCP** (`socket`, `connect`) y **resolución DNS** (`getaddrinfo`): glibc.
- **Armado y lectura de HTTP/1.1**: código propio.
- **Parser JSON mínimo**: código propio (`src/json.c`).

## Requisitos previos

- Linux, o WSL en Windows.
- Compilador `gcc` y `make`.
- Paquetes de desarrollo (Debian/Ubuntu):
  ```bash
  sudo apt install build-essential libssl-dev libsqlite3-dev
  ```
- API Key de Google Cloud con Google Sheets API habilitada y restringida por IP `/16` (ver *Autenticación*).
- Una spreadsheet pública ("Cualquiera con el enlace puede ver").
- Salida a internet por el puerto 443.
- Opcional: la CLI `sqlite3` para inspeccionar la base.

## Compilación y uso

```bash
make                                    # genera ./sheets_client
export SHEETS_API_KEY="tu-api-key"      # la key nunca va en el código ni en el repo
./sheets_client --init-db               # crea las tablas (una sola vez)
./sheets_client "https://docs.google.com/spreadsheets/d/<ID>/edit"
```

## Comandos CLI

| Comando | Descripción |
|---|---|
| `./sheets_client <URL\|ID>` | Consulta la metadata, la valida, la guarda y versiona si cambió el schema. |
| `./sheets_client --init-db` | Crea las tablas `assets` y `assets_history` si no existen. |
| `./sheets_client --db <ruta> ...` | Usa otra base. Por defecto, `db/redes2.db`. |
| `./sheets_client --help` | Muestra la ayuda. |

La API Key se lee de la variable de entorno `SHEETS_API_KEY`.

## Flujo de la aplicación

```text
./sheets_client <URL|ID>
        │
        ▼
Extraer spreadsheetId  →  leer SHEETS_API_KEY
        │
        ▼
Socket TCP a sheets.googleapis.com:443  →  handshake TLS (OpenSSL)
        │
        ▼
GET /v4/spreadsheets/{id}?key=...  (HTTP/1.1)
        │
        ▼
Leer respuesta
   ├── error HTTP (400/401/403/404/429/5xx) → mensaje claro y salida controlada
   └── 200 → parsear JSON
              │
              ▼
        Validar contra el schema fijo
   ├── falta un campo obligatorio o tiene otro tipo → error controlado, no se guarda nada
   └── campos desconocidos → se podan (se avisa por consola)
              │
              ▼
        Calcular schema_hash (SHA-256)
              │
              ▼
   ¿Existe el asset en la base?
   ├── No → INSERT en assets
   ├── Sí, mismo hash → UPDATE de meta_payload y updated_at
   └── Sí, hash distinto → copiar el estado previo a assets_history y luego UPDATE
```

## Modelo de tests

Sin frameworks externos: `assert.h` (glibc) y un `make test`.

| Nivel | Qué cubre |
|---|---|
| **Unitarios** | Extracción del ID (URL válida, ID pelado, entrada inválida). Parser JSON (válido, truncado, mal formado). Validación y poda del schema. Hash estable: misma estructura da el mismo hash, otra cantidad de pestañas da el mismo hash, un campo nuevo o un tipo distinto da otro hash. |
| **Base de datos** | SQLite en memoria (`:memory:`): insert, update sin historial y update con historial. |
| **Con fixtures, sin red** | Respuestas JSON guardadas en `tests/fixtures/`: normal, sin `properties.title`, con un campo nuevo, con un tipo cambiado. |
| **End-to-end manual** | Contra la API real con una spreadsheet pública. Se verifican spreadsheet privada (403), ID inexistente (404) y key inválida (400). Las capturas van a la evidencia. |

## Estructura del proyecto

```text
sheets-client/
├── Makefile
├── README.md
├── contrato.md
├── workplan.md
├── .gitignore              # excluye *.db y cualquier archivo con la key
├── db/
│   └── schema.sql
├── src/
│   ├── main.c              # orquesta el flujo
│   ├── cli.c/.h            # argumentos y extracción del spreadsheetId
│   ├── net.c/.h            # socket TCP + TLS (OpenSSL)
│   ├── http.c/.h           # armado del request y lectura de la respuesta
│   ├── json.c/.h           # parser JSON mínimo
│   ├── schema.c/.h         # validación, poda y schema_hash
│   └── db.c/.h             # SQLite: assets y assets_history
├── tests/
│   ├── test_*.c
│   └── fixtures/
├── Documentacion/
│   ├── 01-BDD.md
│   ├── 02-API_SETUP.md
│   ├── 03-Implementacion.md
│   └── 04-Evidencias.md
└── images/                 # capturas de evidencia
```

## Documentación

| Documento | Contenido |
|---|---|
| [Contrato](contrato.md) | Compromiso formal del proyecto |
| [Workplan](workplan.md) | Tareas, fechas, tiempos y diagrama PERT |
| `Documentacion/01-BDD.md` | Análisis de la base de datos |
| `Documentacion/02-API_SETUP.md` | Configuración de la API Key y la restricción de IP `/16` |
| `Documentacion/03-Implementacion.md` | Arquitectura, artefactos y decisiones |
| `Documentacion/04-Evidencias.md` | Tests, salidas y capturas, un apartado por criterio de aceptación |

De forma opcional, las decisiones de arquitectura se registran como ADR (Architecture Decision Record), uno por decisión.

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
>Click en Save. La key queda lista para usarse como query param en tus requests: ...?key=TU_API_KEY. Guardala en la variable de entorno `SHEETS_API_KEY`, nunca hardcodeada ni en el repo público.


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

### Schema fijo y política ante cambios

Aunque `meta_payload` es polimórfico, solo se acepta guardar una respuesta que cumpla este schema fijo:

| Campo | Tipo | Condición |
|---|---|---|
| `spreadsheetId` | string | **Obligatorio** |
| `properties` | object | **Obligatorio** |
| `properties.title` | string | **Obligatorio** |
| `sheets` | array | **Obligatorio** |
| `sheets[].properties` | object | **Obligatorio** |
| Resto de los campos de las tablas anteriores (`spreadsheetUrl`, `locale`, `timeZone`, `autoRecalc`, `defaultFormat`, `sheetId`, `title`, `index`, `sheetType`, `gridProperties`) | el tipo indicado en cada tabla | Opcionales |

Qué hace el programa si Google cambia el formato:

- **Falta un campo obligatorio, o un campo conocido llega con otro tipo** → finaliza con un mensaje de error claro y un código de salida distinto de 0. No se guarda nada.
- **Aparece un campo desconocido** → se poda (no se guarda), se informa por consola y no genera una versión nueva.
- **Un campo opcional aparece o desaparece** → no es un error: cambia el `schema_hash` y por lo tanto se versiona.

---

## Mapeo a la base de datos

La respuesta del API se proyecta sobre la tabla `assets`:

| Columna        | Origen                                                          |
| -------------- | ---------------------------------------------------------------- |
| `asset_uri`    | `https://docs.google.com/spreadsheets/d/<spreadsheetId>` (`UNIQUE`) |
| `title`        | `properties.title`                                                |
| `entity`       | `"spreadsheet"` (constante)                                       |
| `provider`     | `"google_sheets"` (constante)                                     |
| `schema_hash`  | SHA-256 (hex) de la estructura del payload validado: rutas de campos + tipos, sin valores |
| `meta_payload` | JSON validado de la respuesta (`properties` + `sheets`), sin campos desconocidos           |
| `created_at`   | Gestionado por la DB                                              |
| `updated_at`   | `DEFAULT` al insertar; el programa lo actualiza en cada `UPDATE`                        |

### Versionado de schema

- `assets.meta_payload` mantiene **siempre el JSON vigente** del recurso (ya validado y podado).
- `assets.schema_hash` guarda el SHA-256 de la **estructura** de ese payload: el conjunto
  ordenado de rutas de campos con su tipo JSON, sin valores (ej. `properties.title:string`,
  `sheets[].properties.sheetId:number`). Los elementos de un array se agrupan en una sola
  ruta (`sheets[]`, con la unión de los campos de todos sus elementos), así que agregar o
  borrar pestañas **no** cambia el hash; que un campo opcional aparezca o desaparezca, sí.
- En cada consulta se recalcula el hash y se compara con el guardado:
  - **Igual** → no hubo cambio estructural. Se actualizan `meta_payload` y `updated_at`
    (por ejemplo, si cambió el título), sin generar historial.
  - **Distinto** → cambio estructural. Antes de mutar, el estado previo de la fila
    (con su `schema_hash`) se guarda en `assets_history`, y recién después se actualiza
    `assets`.

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
    schema_hash TEXT NOT NULL
        CHECK(length(schema_hash) = 64), -- SHA-256 (hex) de la estructura del payload vigente
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
  
CREATE TABLE IF NOT EXISTS assets_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    asset_id INTEGER NOT NULL,
    history_payload TEXT NOT NULL
        CHECK(json_valid(history_payload)), -- Snapshot JSON completo de la fila antes de mutar
    schema_hash TEXT NOT NULL
        CHECK(length(schema_hash) = 64), -- Hash de la estructura que tenía ese snapshot
    changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE
);
 
 
CREATE INDEX IF NOT EXISTS idx_assets_history_asset_id
ON assets_history(asset_id);
```
 
Decisiones de diseño del schema:
 
- `schema_hash` en `assets` y `assets_history` — permite detectar cambios estructurales sin comparar los JSON completos.
- `PRAGMA foreign_keys = ON;` — SQLite no valida foreign keys por defecto, hay que activarlo explícitamente en cada conexión.
- `CHECK(json_valid(...))` en `meta_payload` y `history_payload` — rechaza a nivel de base de datos cualquier intento de guardar un JSON corrupto, en vez de descubrirlo recién al leerlo de vuelta.
- `CHECK(entity = ...)` / `CHECK(provider = ...)` — ambos campos son constantes en este proyecto; el CHECK deja eso documentado en el propio schema.
- Índice sobre `assets_history(asset_id)` — SQLite **no crea automáticamente** un índice sobre columnas de foreign key (a diferencia de `asset_uri`, que al ser `UNIQUE` sí tiene su índice implícito). Sin este índice, buscar "todo el historial de un asset" recorre la tabla entera fila por fila.

---

## Estado / Base de datos

La base SQLite es un archivo (por defecto `db/redes2.db`, configurable con `--db`).
Las tablas se crean con `./sheets_client --init-db`, que ejecuta la definición de
`db/schema.sql`; todas las sentencias usan `IF NOT EXISTS`, así que puede correrse más
de una vez sin riesgo. La API Key debe generarse y restringirse por IP (`/16`) en
Google Cloud Console antes de la primera ejecución. La capa de persistencia y el
cliente HTTP/TLS en C se implementan siguiendo el [workplan](workplan.md).