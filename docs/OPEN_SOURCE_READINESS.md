# CQL Open-Source Readiness Report

**Format:** Closing roundtable — architecture/RPC lead, CI/CD lead, security lead, and an
open-source lead, synthesized.
**Date:** 2026-07-17
**Question:** What does it take to take CQL "to the next level" and put it in front of the
open-source community?

> Honest verdict up front: **CQL is not yet ready for a public launch, but it is close on the
> axes that are hardest to fix later (build, CI, test integrity) and the remaining gaps are
> well-scoped.** The blockers are a small number of security fixes and one architectural
> consolidation, plus the contributor-facing paperwork every OSS project needs. None of them are
> research problems; they are a focused 1–2 week runway.

---

## 1. Readiness scorecard

| Axis | Before this pass | Now | Ready to launch? |
|---|---|---|---|
| Builds cleanly for a newcomer | ❌ broken on modern compilers | ✅ green, warning-clean, ASan/UBSan-clean | **Yes** |
| Automated CI | ❌ none (only bots) | ✅ Linux+macOS build/test + sanitizer job | **Almost** — Linux/GCC leg still needs a first green run |
| Test integrity | ❌ 9 mirage tests, 1 disabled, flaky external | ✅ 291 run / 268 pass / 0 disabled, hermetic | **Yes**, with known gaps documented |
| Security posture | ❌ cleartext-key regression, false docs | ⚠️ #1 fixed, docs corrected; SW2–SW11 open | **No** — see §3 |
| Documentation honesty | ❌ SECURITY.md fictional in places; promotional eval doc | ⚠️ SECURITY.md fixed; eval doc still promotional | **No** — see §4 |
| Contributor onboarding | ❌ no CONTRIBUTING, templates, or hook install | ❌ unchanged | **No** — see §5 |
| Architecture a contributor can navigate | ⚠️ duplication traps | ⚠️ unchanged | **No** — see §6 |
| License | ✅ MIT | ✅ MIT | **Yes** |

---

## 2. CI/CD lead

**Done:** the project now has real CI — the single biggest change. A newcomer's PR will actually
be built and tested on two toolchains, and there is a sanitizer job.

**Before launch:**
1. **Get the Linux/GCC leg green.** CI was authored but only the macOS/AppleClang leg is
   locally verified; GCC under `-Werror` may surface warnings AppleClang doesn't. This must be a
   confirmed-green run before the badge goes public.
2. **Release automation.** Tag → build artifacts (at least Linux/macOS binaries) → GitHub Release.
   Newcomers should not have to build from source to try it.
3. **Coverage in CI.** The project claims an 85% target that nothing measures. Add
   `-DENABLE_COVERAGE` (llvm-cov) + a CI job + a coverage badge, or drop the claim.
4. **Supply-chain hygiene.** Pin third-party GitHub Actions (`lukka/get-cmake`) to a commit SHA,
   not a moving tag. Pin the GoogleTest/nlohmann versions (already pinned) and document the policy.

**Nice to have:** a Windows/MSVC leg (the code has `_WIN32` branches but they are untested), and
`clang-tidy`/`cppcheck` in CI.

---

## 3. Security lead — **the hard launch blocker**

An external OSS user will paste a real API key into this tool. That raises the bar. The
independent audit found 0 critical / 3 high / 5 medium issues; one high is fixed. Before a public
launch that invites real keys:

**Must fix (from the security work list):**
- **SW3 — SecureString leak.** The advertised "memory-locked" key protection does not hold end to
  end. Fix the `data()` copy and the cached header map, or stop advertising the guarantee.
- **SW4 / SW5 — DoS caps.** Enforce the response-size limit at the read callback and cap
  generated-file count/size + validate the output path. A hostile or buggy API response should not
  be able to exhaust memory or disk.
- **SW6 — MCP input bounds.** The JSON-RPC server reads unbounded stdin and parses unbounded
  nesting; a crafted payload crashes it. This is the "RPC" attack surface.
- **SW7 — config file `0600`.** Don't persist keys world-readable.

**Resolve the two honesty items** (they read as red flags in a security review):
- **SW9 — remove or clearly caveat the shell/SQL "injection prevention".** It guards a sink that
  does not exist; leaving it named as protection invites a false sense of security in the next
  contributor who adds a real sink.
- **SW10 — the dead `validate_api_key`** (with a no-op branch) should be wired in or deleted.

**Already good:** API keys are redacted in logs; the codebase is memory-clean under ASan/UBSan;
`SECURITY.md` now has a vulnerability-reporting section (make sure the email/contact is real).

---

## 4. Open-source lead — honesty and framing

**The single most important thing for an OSS launch is that the repo tells the truth about
itself**, and this pass moved a long way toward that (SECURITY.md, the CHANGELOG, these reports).
Two things remain:

1. **`docs/CQL_HONEST_EVALUATION.md` is not honest — it is a pitch deck.** Revenue tiers
   ($19/mo, $99/user/mo), "market size $4.4T", "PROCEED" investment language, and star-rating
   feature grids do not belong in an open-source repository; they read as marketing and undercut
   the credibility the rest of the docs are building. Replace it with a factual `ROADMAP.md`
   (what works, what's planned, known limitations) and move any commercial strategy out of the
   public repo.
2. **Reconcile remaining README claims with reality.** The README is much improved, but a reader
   should be able to trust every claim. Cross-check "250+ tests" (true: ~291), "zero warnings"
   (true after this pass), and any capability claims against what the code does.

**Standard OSS scope statement:** add a one-paragraph "what CQL is and isn't" (it is a structured
prompt compiler + submission tool; it is not a general-purpose programming language — the README
already gestures at this; make it prominent).

---

## 5. Contributor onboarding — missing paperwork

None of this is hard; all of it is expected by contributors:
- **`CONTRIBUTING.md`** — build/test instructions (they exist in CLAUDE.md; surface them), the
  commit style, the `-Werror`/test bar, how to run the hook.
- **`.github/ISSUE_TEMPLATE/` + `PULL_REQUEST_TEMPLATE.md`** — the repo has bot workflows but no
  templates.
- **`CODE_OF_CONDUCT.md`** — standard.
- **Auto-install the pre-commit hook.** The EOF-newline hook exists but isn't installed by
  default (`core.hooksPath` unset); add a `scripts/setup-hooks.sh` or a CMake step, and mention it
  in CONTRIBUTING.
- **"Good first issues."** The deferred test/security work list in this pass is a ready-made set of
  well-scoped starter tasks — file them as issues.

---

## 6. Architecture lead — resolve the traps before inviting contributors

A new contributor will trip on the duplication documented in `ARCHITECTURE_REVIEW.md`:
- **Two HTTP clients** — which one do I change? (Already caused one security regression.)
- **Two test harnesses** — the legacy `cql --test` runs `pass()` stubs.
- **Three directive lists** — add a directive and two of them silently disagree.

Resolving the P0s (consolidate to one HTTP client; single directive registry) before a
contribution push will save every future contributor from these traps and remove a class of bugs.
This is the highest-leverage pre-launch engineering work after the security fixes.

---

## 7. Roadmap to "the next level"

**Phase A — launch-ready (1–2 weeks):**
1. Fix security SW3–SW7, resolve SW9/SW10 (§3).
2. Confirm the Linux/GCC CI leg is green; pin action SHAs (§2).
3. Consolidate to one HTTP client; add the single directive registry (§6).
4. Add CONTRIBUTING / templates / CoC / hook installer (§5).
5. Replace the promotional eval doc with a factual ROADMAP; final README truth-pass (§4).

**Phase B — credible v1 (post-launch):**
6. Coverage tooling + badge; fill the `api_client` real-HTTP-path test gap; remove the legacy
   `--test` harness.
7. Release binaries (GitHub Releases) + a package (Homebrew tap, vcpkg/Conan port).
8. Route all output through `UserOutputManager`; finish the async/pooling story.

**Phase C — differentiation (the actual "next level"):**
9. Editor/LSP integration (syntax highlighting, template preview) — the highest-impact adoption
   lever for a DSL.
10. Language features the README honestly lists as absent: conditionals/loops/macros, if the
    community wants them.
11. A template-sharing story (a registry or just a well-organized `examples/` + docs).

---

## 8. Bottom line

CQL has a real domain, a genuinely good compiler core, and — after this pass — a trustworthy
build, CI, and test foundation. It is **not** launch-ready today, but the gap is a concrete,
mostly-mechanical checklist rather than open-ended work: a handful of security fixes, one
architectural consolidation, the standard contributor paperwork, and a final honesty pass on the
docs. Do Phase A and CQL is something you can put in front of the community without flinching.

**Companion documents:** `docs/ARCHITECTURE_REVIEW.md` (architecture deep-dive), `CHANGELOG.md`
("Hardening & CI Pass" — everything changed this cycle), `docs/SECURITY.md` (now accurate).
