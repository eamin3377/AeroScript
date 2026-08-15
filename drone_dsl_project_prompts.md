# Drone Movement DSL — Compiler Project Build Plan
### (flex + bison backend, modern web UI frontend)

Registered topic: **#8 Robot Movement DSL** (implemented as a 3D drone flight simulator)

---

## Architecture

```
program.drone (source file)
      │
      ▼
 ┌─────────┐   tokens   ┌─────────┐   AST   ┌──────────────┐   JSON   ┌────────────┐
 │  FLEX   │ ─────────► │ BISON   │ ──────► │ Interpreter/  │ ───────► │  Web UI    │
 │ lexer.l │            │ parser.y│         │ Semantic      │          │ (browser)  │
 └─────────┘            └─────────┘         │ Analyzer (C)  │          └────────────┘
                                             └──────────────┘
```

- **flex** tokenizes the `.drone` source into tokens
- **bison** parses tokens into an AST using a grammar (this is your real "parser" deliverable)
- A **C interpreter** walks the AST, runs semantic checks, simulates drone state, and prints a **JSON flight log** to stdout
- A **web UI** (HTML/CSS/JS) loads that JSON and animates the flight — clean, colorful, minimal, with motion

This mirrors a real compiler pipeline (lexer → parser → semantic analysis → codegen/output), which is exactly what your instructor is grading.

---

## Full DSL Command Reference (implement ALL of these)

| Command | Syntax | Meaning |
|---|---|---|
| Takeoff | `TAKEOFF` | Drone becomes airborne |
| Land | `LAND` | Drone lands, altitude resets to 0 |
| Move | `MOVE FORWARD <n>` / `BACKWARD` / `LEFT` / `RIGHT` <n> | Horizontal movement, n = meters |
| Ascend/Descend | `MOVE UP <n>` / `MOVE DOWN <n>` | Vertical movement |
| Turn | `TURN LEFT <deg>` / `TURN RIGHT <deg>` | Rotate heading |
| Hover | `HOVER <seconds>` | Pause in place, still drains battery |
| Repeat | `REPEAT <n> [ <statements> ]` | Loop block n times |
| Set speed | `SET SPEED <n>` | Changes distance-per-tick for subsequent moves |
| Return home | `RETURN_HOME` | Auto-navigate back to (0,0,0) |
| Battery config | `SET BATTERY <percent>` | Initialize starting battery (optional, defaults to 100) |

**Semantic rules to enforce (this is your "semantic analysis" phase — examiners look for this):**
- Any `MOVE`/`TURN`/`HOVER` before `TAKEOFF` → error
- Double `TAKEOFF` while already airborne → error
- Altitude ceiling (e.g. 30m) → error if exceeded
- Battery hits 0 mid-flight → error
- `REPEAT` block not closed with `]` → parse error with line number

---

## Phase 1 — Flex Lexer

**Copy this prompt to your AI tool:**

```
Write a flex (.l) lexer file named lexer.l for a Drone Movement DSL used in a
compiler design course project. The lexer must tokenize the following:

Keywords (case-insensitive, return as distinct token types):
TAKEOFF, LAND, MOVE, TURN, REPEAT, HOVER, FORWARD, BACKWARD, UP, DOWN,
LEFT, RIGHT, SET, SPEED, BATTERY, RETURN_HOME

Symbols: '[' and ']' as LBRACKET and RBRACKET
Numbers: integers and decimals, return as NUMBER with yylval as double
Identifiers: not needed beyond keywords, but include an IDENT fallback rule
Comments: lines starting with // should be ignored
Whitespace/newlines: ignored but track line numbers using a global int yylineno
  (enable %option yylineno)
Unknown characters: print a lexical error with the line number and character,
  then continue scanning (don't crash)

Requirements:
- Use %option noyywrap
- Token type constants should be defined in a header "tokens.h" that both
  this lexer and a future bison parser will #include (use an enum)
- Include a small main() guarded by #ifdef LEXER_TEST that reads a file
  passed as argv[1] and prints each token type + value, so I can test the
  lexer standalone before wiring up the parser
- Add comments explaining each rule for someone new to flex

Output: the full lexer.l file and the tokens.h header file.
```

**Test before moving on:** compile with `flex lexer.l && gcc -DLEXER_TEST lex.yy.c -o lexertest -lfl` and run it on a sample `.drone` file. Confirm every token prints correctly.

---

## Phase 2 — Bison Parser + AST

**Copy this prompt to your AI tool** (attach/paste your working `lexer.l` and `tokens.h` from Phase 1 first):

```
Here is my working flex lexer (lexer.l) and tokens.h [paste them].

Now write a bison (.y) grammar file named parser.y for the same Drone
Movement DSL. It must:

1. Define an AST using C structs (a tagged union or a base struct + node
   type enum) with node types: Program, Takeoff, Land, Move, Turn, Hover,
   Repeat, SetSpeed, SetBattery, ReturnHome. Repeat nodes contain a list of
   child statement nodes (use a linked list or dynamic array).

2. Grammar rules for:
   program        -> statement_list
   statement       -> TAKEOFF | LAND | move_stmt | turn_stmt | hover_stmt
                     | repeat_stmt | set_speed_stmt | set_battery_stmt
                     | RETURN_HOME
   move_stmt       -> MOVE direction NUMBER   (direction: FORWARD/BACKWARD/UP/DOWN/LEFT/RIGHT)
   turn_stmt       -> TURN (LEFT|RIGHT) NUMBER
   hover_stmt      -> HOVER NUMBER
   repeat_stmt     -> REPEAT NUMBER LBRACKET statement_list RBRACKET
   set_speed_stmt  -> SET SPEED NUMBER
   set_battery_stmt-> SET BATTERY NUMBER

3. On any syntax error, implement yyerror() to print the line number
   (use yylineno) and a clear message, then continue if possible so
   multiple errors can be reported in one pass (don't just abort on first
   error — accumulate errors in a global list).

4. Build the AST as parsing proceeds ($$ = makeXNode(...)) and store the
   final Program node in a global `ASTNode* ast_root`.

5. Include a main() that calls yyparse(), then calls a function
   print_ast(ast_root, 0) that pretty-prints the AST indented by nesting
   level, so I can verify the tree structure from the command line.

Output: the full parser.y file, an ast.h header with all struct
definitions, and ast.c with the node-construction and print_ast functions.
```

**Test before moving on:** `bison -d parser.y && flex lexer.l && gcc lex.yy.c parser.tab.c ast.c -o parsertest -lfl` and run on your sample program. Confirm the printed AST correctly nests the `REPEAT` block contents.

---

## Phase 3 — Semantic Analysis + Interpreter (JSON output)

**Copy this prompt to your AI tool** (attach your `ast.h`/`ast.c` from Phase 2):

```
Here is my AST definition (ast.h, ast.c) [paste them] built from a bison
parser for a Drone Movement DSL.

Write interpreter.c that:

1. Defines a DroneState struct: x, y, z (doubles), heading (0-359 degrees),
   speed (default 1.0), battery (default 100.0), flying (bool).

2. Walks the AST recursively (a function `void exec(ASTNode* node, DroneState* s, FILE* jsonOut, ErrorList* errs)`)
   and for each executed primitive command (Takeoff, Land, Move, Turn,
   Hover — NOT Repeat itself, but each iteration's children) appends one
   JSON object to a growing array representing a "flight frame":
   {"cmd":"MOVE FORWARD","x":..,"y":..,"z":..,"heading":..,"battery":..,"flying":true}

3. Enforces these semantic rules, appending to ErrorList with line numbers
   instead of crashing:
   - MOVE/TURN/HOVER before TAKEOFF
   - double TAKEOFF while already flying
   - altitude > 30 meters (clamp to 30, record as error)
   - battery reaching 0 while still flying
   - REPEAT with a negative or zero count (warn, treat as 0 iterations)

4. Battery drains by (distance_moved * 1.2) for MOVE, and 0.5 per second
   for HOVER. SET SPEED scales distance for subsequent MOVE commands
   (e.g. MOVE FORWARD 5 with SPEED 2 covers 10 units).

5. RETURN_HOME computes a straight-line move back to (0,0, current_z) and
   emits a single frame for it.

6. After execution finishes, output a single JSON object to stdout (or a
   file given as argv[2]) shaped like:
   {
     "frames": [ ...as above... ],
     "errors": ["Line 4: MOVE FORWARD issued before TAKEOFF", ...],
     "final_state": {"x":..,"y":..,"z":..,"battery":..}
   }
   Write this by hand with fprintf (no external JSON library needed —
   keep dependencies to the standard library only).

7. Write main.c that ties it together: argv[1] = input .drone file,
   argv[2] = optional output json path (default stdout). Pipeline:
   flex tokenizes -> bison parses into AST -> interpreter walks AST ->
   JSON printed.

Output: interpreter.c, interpreter.h, and main.c.
```

**Test before moving on:** run the full pipeline on your sample program and confirm valid JSON comes out (paste it into a JSON validator). Deliberately write a broken program (e.g. `MOVE FORWARD 5` before `TAKEOFF`) and confirm the `errors` array reports it with the right line number.

---

## Phase 4 — Build System

**Copy this prompt to your AI tool:**

```
I have these C/flex/bison files: lexer.l, parser.y, ast.h, ast.c,
interpreter.h, interpreter.c, main.c, tokens.h [list them / paste if needed].

Write a Makefile that:
- Runs bison -d on parser.y to generate parser.tab.c and parser.tab.h
- Runs flex on lexer.l to generate lex.yy.c
- Compiles everything into a single executable called `dronec`
- Links against -lfl (or -ll depending on platform) as needed
- Has a `make clean` target
- Has a `make test` target that runs ./dronec on a sample.drone file in a
  /examples folder and prints the resulting JSON

Also write 3 example .drone programs in /examples: one valid simple
flight, one using REPEAT and SET SPEED, and one that deliberately
triggers 2 different semantic errors (for demo purposes).

Output: Makefile and the three example .drone files.
```

**Test before moving on:** `make && make test` should produce clean JSON with no compiler warnings. This `dronec` binary is your actual "compiler" deliverable for the course — keep it in your submission.

---

## Phase 5 — Web UI (modern, clean, colorful, animated)

**Copy this prompt to your AI tool:**

```
Build a single-file HTML/CSS/JS web app that visualizes a drone flight
from a JSON file shaped like:
{
  "frames": [{"cmd":"MOVE FORWARD","x":0,"y":5,"z":4,"heading":0,"battery":94,"flying":true}, ...],
  "errors": ["Line 4: ..."],
  "final_state": {"x":0,"y":0,"z":0,"battery":80}
}

Design direction — IMPORTANT, follow exactly:
- Light, modern, professional SaaS-dashboard aesthetic. NOT a dark/black
  theme, NOT a "hacker terminal" look.
- Color palette: soft off-white background (#F7F9FC), primary accent a
  confident blue (#3B6FE0), secondary accent a warm coral (#FF6B5D) used
  sparingly for warnings/errors, success green (#2FBF71), text dark slate
  (#1E2733), muted gray (#8A94A6). Use soft shadows and rounded corners
  (12-16px radius) on cards, not hard borders.
- Typography: a clean modern sans-serif (Inter or similar via Google
  Fonts) for everything — a confident weight-600 for headings, regular
  for body/data.
- Layout: a top nav bar with the project title, a left card for "Upload
  flight plan (.drone or .json)" and a Run/Play button, a center card
  containing an animated 2.5D or 3D flight path (use three.js via CDN,
  camera looking down at a slight angle at a grid floor, drone rendered
  as a simple friendly rounded icon, NOT a harsh military drone), and a
  right card showing a live telemetry readout (altitude, battery as a
  smoothly animated progress bar, heading as a rotating compass icon)
  plus a scrollable "Flight events" list that highlights the current
  frame as playback proceeds.
- Motion: animate the drone smoothly interpolating between frames (don't
  jump-cut), animate card entrances with a subtle fade+slide on load,
  animate the battery bar color transition (green -> amber -> coral) as
  it depletes, and add a subtle pulsing glow on the drone icon while
  "flying". Keep motion tasteful and purposeful, not flashy.
- If "errors" is non-empty, show a dismissible coral-colored banner
  listing them clearly, and mark the corresponding step in the "Flight
  events" list with a small warning icon.
- Fully responsive down to a laptop screen width at minimum.
- No external JS framework needed — plain HTML/CSS/JS + three.js from
  cdnjs is fine. Everything in one HTML file.

The user will either paste JSON directly into a textarea, or (if serving
over a local server) fetch a flight.json file via a "Load latest" button
that does fetch('./flight.json'). Support both.

Output: the complete single index.html file.
```

**Test before moving on:** run your `dronec` compiler on a sample program, redirect its JSON output to `flight.json`, then open `index.html` and paste that JSON in — confirm the animation plays correctly and errors surface clearly.

---

## Phase 6 — Wiring the C backend to the browser (pick ONE)

**Option A — Simplest for a live demo (recommended):**
Run `./dronec program.drone flight.json` in a terminal before your demo, then open `index.html` and click "Load latest" (or paste the JSON). Zero extra tooling. Good enough for a lab defense — you show the terminal compiling, then show the browser animating.

**Option B — One-click "Compile & Fly" button (more impressive, more setup):**

```
I have a working command-line compiler `dronec` (built from flex+bison+C)
that takes a .drone file and outputs flight JSON, and a web UI
(index.html) that visualizes that JSON.

Write a minimal local Node.js server (server.js, using only the built-in
http and child_process modules, no npm dependencies) that:
- Serves index.html and static files from the current directory
- Exposes a POST /compile endpoint that accepts raw .drone source text in
  the request body, writes it to a temp file, runs `./dronec tempfile.drone`
  as a child process, captures its stdout (the JSON), and returns it as
  the HTTP response with Content-Type application/json
- Handles the case where dronec exits with a non-zero code (still try to
  return whatever JSON/error text it produced)

Also update index.html to add a code editor textarea where I can type or
paste .drone source directly, with a "Compile & Fly" button that POSTs to
/compile and then feeds the returned JSON straight into the existing
visualization/animation code.

Output: server.js and the diff/additions needed in index.html.
```

Run with `node server.js`, open `http://localhost:3000`. This gives you the full "type code → click button → watch it fly" demo in one browser window, which is the strongest possible way to defend this project live.

---

## Suggested order of work

1. Phase 1 (lexer) — test standalone
2. Phase 2 (parser + AST) — test standalone, verify AST printout
3. Phase 3 (interpreter/JSON) — test standalone, verify JSON + error cases
4. Phase 4 (Makefile) — confirm `make && make test` is clean
5. Phase 5 (UI) — test with a hand-made JSON file first
6. Phase 6 — wire it together, pick Option A or B depending on how much time you have before submission

Do them in order — each phase's prompt assumes the previous one's files exist and works. If Phase 3 needs your Phase 2 AST changed slightly (field names, etc.), just tell the AI tool what you actually ended up with rather than what the prompt assumed.
