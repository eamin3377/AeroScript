# AeroScript — Drone Movement DSL & Flight Simulator

**AeroScript** is a domain-specific language (DSL) and interactive 3D flight simulation platform designed as a university Compiler Design project. It translates high-level drone movement commands into 3D flight trajectories and real-time telemetry.

---

## 🏗️ Compiler Architecture & Pipeline Workflow

The compilation pipeline follows standard compiler design stages:

```
┌─────────────────┐       ┌─────────────────┐       ┌──────────────────┐
│  program.drone  │ ────► │   Flex Lexer    │ ────► │   Bison Parser   │
│  (Source Code)  │       │   (lexer.l)     │       │    (parser.y)    │
└─────────────────┘       └─────────────────┘       └──────────────────┘
                                                             │
                                                             ▼
┌─────────────────┐       ┌─────────────────┐       ┌──────────────────┐
│     Web UI      │ ◄──── │   Flight JSON   │ ◄──── │   Interpreter /  │
│  (Three.js 3D)  │       │  (stdout/file)  │       │ Semantic Checks  │
└─────────────────┘       └─────────────────┘       └──────────────────┘
```

---

## 🔬 How Each Component Works

### 1. Flex Lexical Analyzer (`lexer.l` & `tokens.h`)
- **Role**: Scans the raw `.drone` source code character-by-character and breaks it into terminal tokens.
- **Key Implementation Details**:
  - Uses `%option caseless` so commands like `TAKEOFF`, `takeoff`, or `TakeOff` are all valid.
  - Recognizes keywords (`TAKEOFF`, `LAND`, `MOVE`, `TURN`, `REPEAT`, `HOVER`, `SET`, `SPEED`, `BATTERY`, `RETURN_HOME`, directions).
  - Uses regular expressions `[0-9]+(\.[0-9]+)?` to parse numbers and converts them via `atof()`.
  - Tracks exact line numbers (`yylineno`) for detailed compiler error diagnostics.
  - Ignores single-line comments starting with `//` and whitespace.

### 2. Bison Parser & Syntax Analysis (`parser.y`)
- **Role**: Parses the token stream against a Context-Free Grammar (CFG) and constructs an Abstract Syntax Tree (AST).
- **Key Implementation Details**:
  - Defines grammar rules for single statements and nested blocks (`REPEAT <n> [ <statements> ]`).
  - Implements error recovery routines (`yyerror()`) that capture syntax errors along with line numbers without crashing immediately.
  - Constructs memory-allocated AST nodes during the parsing pass and assigns the root node to `ast_root`.

### 3. Abstract Syntax Tree (AST) Data Structures (`ast.h` & `ast.c`)
- **Role**: Provides a clean, hierarchical C memory representation of the parsed program structure.
- **Key Implementation Details**:
  - Uses tagged union structures (`ASTNode`) representing command types (`AST_MOVE`, `AST_TURN`, `AST_REPEAT`, etc.).
  - Supports dynamic nested statement lists for loop bodies.
  - Includes AST utility functions for statement node creation, node appending, memory cleanup (`free_ast`), and pretty-printing tree representations to stdout.

### 4. Semantic Analysis & Execution Engine (`interpreter.h` & `interpreter.c`)
- **Role**: Performs context-sensitive semantic validation, simulates drone physics, and produces JSON flight logs.
- **Enforced Course Semantic Rules**:
  - ❌ **Pre-flight Check**: Rejects `MOVE`, `TURN`, or `HOVER` commands issued before `TAKEOFF`.
  - ❌ **Double Takeoff Check**: Flags consecutive `TAKEOFF` commands if already airborne.
  - ❌ **Altitude Ceiling Limit**: Enforces a maximum altitude limit of 30 meters.
  - ⚡ **Battery Discharge Model**:
    - Movement consumes `distance * 1.2%` battery.
    - Hovering consumes `0.5% / second`.
    - Triggers error if battery reaches 0% mid-flight.
  - 🏠 **Auto-Navigation (`RETURN_HOME`)**: Computes straight-line vector back to `(0, 0, current Z)`.
- **JSON Generation**: Writes structured JSON output containing:
  - `frames`: Array of state coordinates `(x, y, z, heading, battery, speed, flying)`.
  - `errors`: Array of semantic diagnostic messages with line numbers.
  - `final_state`: Summary of final telemetry coordinates.

### 5. Compiler Executable Driver (`main.c` & `Makefile`)
- Ties Flex scanner (`yylex`) and Bison parser (`yyparse`) together.
- Accepts input `.drone` file path as `argv[1]` and output JSON file path as `argv[2]`.
- Compiled using `gcc` and linked with `-lm` (math library).

### 6. Node.js HTTP Bridge Server (`server.js`)
- Zero-dependency Node.js server that serves web assets.
- Exposes a `POST /compile` endpoint that receives raw `.drone` source code, invokes `dronec.exe` in a child process, and returns JSON output to the frontend.

### 7. Modern 3D Web Simulator (`index.html`)
- Built with HTML5, CSS3, JavaScript, and Three.js.
- **Smooth Animation System**: Uses 60 FPS continuous linear and angular interpolation (`lerp`) for smooth drone motion.
- **Interactive UI**: Includes live telemetry, compass heading indicator, color-coded battery bar, line-highlighted semantic diagnostic cards, and timeline playback controls.

---

## 📖 Complete DSL Command Reference

| Command | Syntax Example | Description |
|---|---|---|
| **Takeoff** | `TAKEOFF` | Drone ascends to 2.0m takeoff altitude |
| **Land** | `LAND` | Drone descends to ground (0.0m) |
| **Move** | `MOVE FORWARD 10` | Moves horizontal or vertical (`FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`, `UP`, `DOWN`) |
| **Turn** | `TURN RIGHT 90` | Rotates heading angle in degrees (`LEFT`, `RIGHT`) |
| **Hover** | `HOVER 3` | Pauses in place for specified seconds |
| **Repeat Loop** | `REPEAT 4 [ ... ]` | Loops enclosed block `N` times |
| **Set Speed** | `SET SPEED 2.0` | Multiplies movement distance per step |
| **Set Battery** | `SET BATTERY 100` | Initializes battery percentage |
| **Return Home** | `RETURN_HOME` | Returns directly to origin coordinate `(0, 0, Z)` |

---

## ⚡ Quick Start Guide

### 1. Build Compiler Executable (`dronec.exe`)

#### On Linux / macOS:
```bash
make
```

#### On Windows (PowerShell / Command Prompt):
```powershell
$env:BISON_PKGDATADIR="C:/Progra~2/GnuWin32/share/bison"
$env:M4="C:/Progra~2/GnuWin32/bin/m4.exe"
& "C:/Progra~2/GnuWin32/bin/bison.exe" -d parser.y
& "C:/Progra~2/GnuWin32/bin/flex.exe" lexer.l
gcc -Wall -Wextra -I. -o dronec.exe lex.yy.c parser.tab.c ast.c interpreter.c main.c -lm
```

### 2. Run Compiler Standalone via CLI

```bash
./dronec.exe examples/valid_flight.drone flight.json
```

### 3. Launch Web Server & 3D Flight Simulator

```bash
node server.js
```

Open **`http://localhost:3000`** in your web browser. Write or select `.drone` flight plans, click **Compile & Fly**, and watch the smooth 3D flight execution in real time!
