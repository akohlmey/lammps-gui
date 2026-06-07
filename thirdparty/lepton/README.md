# Lepton (vendored subset)

This is a reduced, vendored copy of the [Lepton](https://github.com/openmm/openmm)
expression parser, taken from the copy bundled with LAMMPS at `lib/lepton/`
(which originates from the OpenMM project by Peter Eastman et al.).

It is used by LAMMPS-GUI for parsing, evaluating, and symbolically
differentiating user-supplied mathematical expressions (e.g. custom
plot functions and custom fit models).

## License

Lepton is distributed under a permissive MIT-style license (see `LICENSE`),
which is compatible with the GPLv2+ license of LAMMPS-GUI.

## What was kept vs. removed

Only the **core** functionality needed by LAMMPS-GUI is vendored here:

- `Parser`            -- parse an expression string into an AST
- `ParsedExpression`  -- the parsed expression: `evaluate()`, `optimize()`,
                         and analytic `differentiate()`
- `ExpressionTreeNode`, `Operation` -- the AST node types
- `ExpressionProgram` -- an interpreted (stack-machine) evaluator
- `Exception`, `CustomFunction` -- support types

The following was **removed**, because LAMMPS-GUI only needs interpreted
evaluation and symbolic differentiation, not native code generation:

- the bundled `asmjit/` JIT assembler library (~3.8 MB)
- `CompiledExpression` and `CompiledVectorExpression` (the asmjit-based
  just-in-time evaluators)

Accordingly, `Lepton.h` no longer includes `CompiledExpression.h`, and the
`ParsedExpression::createCompiledExpression()` /
`createCompiledVectorExpression()` methods were dropped from
`ParsedExpression.{h,cpp}`. No other source was modified.

## TODO before use in the GUI build

LAMMPS itself contains a (full) copy of Lepton. To avoid duplicate-symbol /
ODR clashes when the GUI is linked against or dynamically loads `liblammps`,
this subset must be moved into a dedicated namespace (e.g. `lammpsgui::lepton`)
before it is added to the GUI build.
