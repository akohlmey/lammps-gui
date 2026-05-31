# AI Assistant for [SOFTWARE_NAME] — Design Digest

> Status: exploratory. This document captures architectural decisions and
> rationale from an early design discussion. It is the durable "memory" for
> work on the AI-assistant feature. Update it as decisions firm up.

## Project context

- **Goal:** add an AI assistant ("chatbot") to the GUI frontend of a complex
  **physics-simulation** application.
- **Frontend:** Qt ([CONFIRM: C++ Qt Widgets / QML / PySide6 / PyQt6]).
- **Approach:** start with a *minimal* implementation on a temporary feature
  branch to explore workflow options, then decide on a fuller architecture and
  implement it interactively.
- **Guiding principle:** the assistant produces a **starting point**, not a
  validated solution. Structural correctness comes from human-vetted artifacts
  and from executable verification — never from the model's unaided judgment.

## Decisions to fill in before serious work

- [ ] Target language of the real software / binding choice.
- [ ] Simulation input-file format + a written spec or worked examples.
- [ ] Which LLM provider(s) to support first.
- [ ] Vector store choice (default candidate: `sqlite-vec`, single local file).
- [ ] Where the workflow-template and remedy libraries live in the repo.

---

## 1. User-selectable model / API / key

- Provide a **provider abstraction layer**; all requests route through one path.
- Settings UI lets the user supply: endpoint URL, model name, optional API key.
- Most providers and local runtimes share the OpenAI-compatible
  `/chat/completions` dialect — OpenAI cloud, OpenRouter, **Ollama**,
  **LM Studio**, **llama.cpp** server. So "paid cloud account", "free
  aggregator", and "local model, no key" are the *same code* with a different
  base URL + model + key.
- Anthropic and Google Gemini use different request/response shapes → write a
  small per-provider adapter that normalizes to/from an internal message format.
- **Do not store keys in plaintext.** Use an OS keychain wrapper (e.g.
  QtKeychain → macOS Keychain / Windows Credential Store / libsecret), not
  QSettings.

## 2. Domain knowledge from preferred sources → RAG, not fine-tuning

- Use **RAG (retrieval-augmented generation)**, not fine-tuning, to teach facts.
  (Fine-tuning is for style/format, is expensive, and is unreliable for recall.)
- Pipeline: chunk curated docs → embed → store vectors → at query time embed the
  question, retrieve top-k chunks, inject with an instruction to **answer only
  from provided sources and say so when they don't cover it**.
- The RAG corpus is the **curated, vetted, annotated** material — not raw docs.
- Retrieval yields **citations** for free; feed those into reliability (below).
- Desktop-friendly store: `sqlite-vec` (one local file); scale to Qdrant/Chroma
  later if needed.

## 3. Reliability / confidence — ground it, don't ask for it

- **A model's self-reported confidence is poorly calibrated.** Do not present a
  verbalized "X% sure" number as if it were a real probability.
- Trustworthy reliability signals, best first:
  1. **Grounding / traceability** — cite which source each claim came from; show
     the underlying passage; instruct it to flag unsupported answers.
  2. **External verification** — check anything checkable deterministically
     rather than asking the model (see §5, the probe loop — this is huge here).
  3. **Self-consistency** — sample N answers; wide disagreement = real
     uncertainty.
  4. **Token logprobs** — weak but genuine signal where the API exposes them.
- Implement honest caveating ("not in my sources") as UX; architect actual
  reliability around grounding + verification.

## 4. File generation → tool/function calling, app does the I/O

- Pattern is **agentic / tool calling**. The model does *not* download anything
  itself; it emits structured content and **the app** handles files.
- Expose app capabilities as callable functions, e.g. `generate_input_file`,
  `validate_file`, `load_into_gui`, `run_simulation`. Give the model the
  file-format spec + a couple of worked examples in context.
- Loop: model calls `generate_input_file` → app writes it → app **validates**
  (parse, schema-check, dry-run) → only then loads into GUI; on failure, feed
  the error back and let the model revise (self-correcting).
- **The LLM cannot guarantee physical correctness.** The validation layer is the
  whole safety net, not optional.
- **Security:** treat model output as untrusted input. If the file format can
  embed scripts or trigger execution, sandbox it; never run generated content
  with full privileges unvetted.

---

## 5. Wizard / expert-system architecture (core idea)

The strongest framing: a classic **expert system**, with an LLM filling the soft
parts that made old rule-based systems brittle.

### Author time vs runtime split (the key move)

- **Author time (offline, expert + LLM):** digest example files and tutorials
  into a **versioned library of vetted workflow templates**. Each template
  carries metadata: when to use, prerequisites, unit assumptions, typical
  parameter ranges, known pitfalls, and citations to source material. The expert
  reviews/corrects before anything enters the library. Output is a static,
  governed knowledge base — **not** a live generator.
- **Runtime (novice user):** the user never asks the LLM to *design* a
  simulation (unreliable). The LLM only **picks among and adapts known-good
  templates** based on the user's answers (reliable).

### The wizard itself

- **Mostly deterministic.** Model type, unit system, workflow type are real
  constraints with dependencies → encode as a decision tree / state machine with
  validation. The user cannot construct an invalid combination. (e.g. choosing
  the model may lock/collapse the unit system.)
- **LLM adds value at the fuzzy seam:** as an *intent interpreter* mapping the
  user's plain-language description onto the valid constrained options, and as an
  *explainer* that justifies each choice (good for novices).

### Handoff to free-form

- Explicit **phase transition**, not a blur. Carry established state forward
  (model, units, workflow, current file) into the open-ended phase's context.
- Relaxing conversational guardrails must **not** relax structural invariants —
  locked unit system / model type stay enforced and validated so refinements
  can't silently break them.

### Cross-cutting

- Provide an **expert-mode escape hatch** to skip the wizard.
- Keep **"starting point, not a validated solution"** explicit in the UI, with
  **provenance** (which template + which sources produced this skeleton).
- Biggest hidden cost: **template maintenance.** Version templates against
  software releases; treat the library as governed, not one-time.

---

## 6. Probe loop — executable verify-and-repair

Use the **simulator itself as a ground-truth oracle**, made cheap by
**downscaling** to a tiny/fast probe. Structure as a bounded agentic loop with
two escalating tiers (like fast→slow CI stages).

### Tier 1 — syntax / runnability (error class a)

- Run the probe for a **single step**; outcome is binary (steps or throws).
- Non-destructive retry: catch exception → extract message → destroy the sim
  instance → instantiate clean → patch input → rerun, until one step succeeds.
- Treat each attempt as a **pure function: input-config → outcome** (no state
  carried between attempts) → reproducible, loggable, safe to automate.
- For known error signatures: **deterministic lookup table** (no LLM). LLM only
  for the unfamiliar long tail.

### Tier 2 — adequate settings (error class b)

- Run N steps; evaluate **explicit, quantitative, machine-checkable quality
  gauges** (energy drift, residual convergence, no blow-up, etc.). The **expert
  defines the gauges**; the LLM operates within them and never invents them.
- On violation: classify symptom (divergence / oscillation / drift) → propose a
  remedy from the curated **symptom→remedy library** (insert steps, adjust
  timestep, change solver setting).

### Safety in the loop

- **Bound iterations** (max cap).
- **Detect thrashing/oscillation** (fix A → symptom B → fix reintroduces A) and
  stop.
- **Explicit give-up / escalate path** to expert or free-form mode.

### Scale-up caveat (be honest)

- Probe-validated settings **don't always transfer** to full geometry (timestep
  stability can depend on mesh resolution, boundary/multiscale effects).
- Scale-up needs its **own** validation pass, not blind inheritance. Annotate
  remedies as **scale-invariant vs resolution-dependent** in the library.

---

## 7. Learning the policy — case-based reasoning, not hand-coded trees

Goal: learn "which choice for what" from data, avoiding brittle `if X then Y/Z`
trees *and* avoiding opaque fine-tuning.

- Use **case-based reasoning** = RAG over *experience*. Accumulate episodes:
  `(situation, symptom, action taken, outcome)`. At runtime, retrieve the most
  similar past cases; LLM reasons by analogy.
  - Learns by accretion, never goes brittle, stays **inspectable & correctable**.
  - Expert's role shifts from *authoring rules* to **curating cases** (lighter).
- **Two data sources, treated differently:**
  - **Forum / mailing list:** rich but **noisy and often wrong**. Mine into
    candidate `(problem, remedy, outcome)` triples. **Validate via the probe
    harness** — re-run reproducible scenarios to confirm which remedies actually
    work. This converts unverified lore into **verified cases** and is what makes
    mining safe. (Also check the data's usage terms.)
  - **Randomized rollouts** = automated curriculum / self-play. Sample configs,
    run the probe loop, log what fixed what. Outcomes **verified by
    construction**; covers space no forum discussed.
- **Keep memory external & retrievable** — don't fine-tune it into weights;
  preserve provenance ("why did it suggest this?"). A lightweight learned
  classifier over symptom features is a *later* optimization, not the foundation.
- **Cold start:** bootstrap from the authored remedy library + synthetic
  rollouts; learned memory takes over as it grows.
- **Drift guard:** the probe-validation gate prevents self-reinforcing errors
  from re-learned outputs.

### Virtuous cycle (the payoff)

Probe loop **generates** verified episodes → **populate** case memory →
**improves** the first-guess remedy → **fewer iterations** → cheaper → **more
rollouts** → bigger memory. Expert sits at the one gate that matters: curating
trustworthy cases.

---

## 8. Key schemas to pin down next (the contracts everything hangs on)

1. **Workflow-template schema** — metadata fields, parameter slots the LLM
   fills, guardrail annotations.
2. **Episode / case record** — fields that capture situation, symptom, action,
   outcome well enough to retrieve and reason over.
3. **Quality-gauge specification** — how the expert declares machine-checkable
   convergence/quality criteria.

## 9. Staged implementation plan

1. **Minimal (this feature branch):** basic provider abstraction + one working
   chat round-trip wired into the GUI; explore workflow options manually.
2. **Decide architecture:** lock the three schemas (§8); choose provider(s),
   vector store, library locations.
3. **Build interactively:** wizard (deterministic core + LLM seam) → template
   library → probe loop (tier 1, then tier 2) → case memory → learning cycle.
