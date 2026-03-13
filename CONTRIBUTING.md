# Contributing to Gorgon Library

Thank you for your interest in contributing to the Gorgon Library. We welcome contributions of all kinds — bug reports, documentation improvements, tests, and code. This document explains the standard contribution process and a few project-specific rules.

## Donation of Contributions

All code submitted to the Gorgon Library is donated to DarkGaze.Org. DarkGaze.Org guarantees that the souce code of the Gorgon Library will always be available under the GNU General Public License (GPL) or any less restrictive license the project may choose, and DarkGaze.Org will keep contributor names publicly available.

By contributing, you acknowledge this donation and agree that DarkGaze.Org will be the recipient of the contributed code.

## Attribution

We maintain a public record of contributors. Unless you request otherwise in writing, your GitHub username (and any name you provide in commits) will be kept public as part of the project history and contributor lists.

## Naming Conventions

Gorgon uses PascalCase for identifiers. Please use PascalCase for all public types, classes, functions, methods, and constants. Local/private/restricted variables should use lowercase names.

Private/internal identifiers may follow project or language idioms, but consistency within the repository is strongly encouraged.

## Documentation (Doxygen)

Every function and class MUST be documented using Doxygen-style comments. Documentation should include a brief description, any side effects, and exceptions/errors that may be raised. Explain parameters and return values, however, using parameter/return documentation can be omitted if description covers them. Make class-level notes on ownership, lifetime, thread-safety, or important invariants where appropriate.

Examples:

C/C++ style (block comment):
/**
 * Brief description of the function.
 *
 * @param[in] value Description of the parameter.
 * @return Description of the return value.
 */
int CalculateValue(int value);

C++/C# style (triple-slash):
/// Brief description of the method.
int CalculateValue(int value);

Make sure class declarations include a Doxygen-style summary and any relevant remarks. You should consider adding a short usage example too.
/// Represents a simple calculator.
/// @note Not thread-safe.
class Calculator { ... };

Add to Examples directory if possible. Examples should be extensively commented to ensure it could be used to learn about your contribution.

## How to Contribute

1. Fork the repository and create a branch named something like `feature/my-feature` or `fix/issue-123`.
2. Implement your change, following the naming and documentation rules above.
3. Add or update tests (unit or manual) where applicable.
4. Run the project's tests and linters locally and ensure they pass.
5. Commit with a clear, descriptive message and push your branch to your fork.
6. Open a pull request against `darkgazeorg/Gorgon` with a clear description of the change, the motivation, and any relevant issue references. Include screenshots or logs if they help reviewers.

## Pull Request Guidelines

- One logical change per pull request makes reviews faster and easier to merge.
- Describe the problem, the chosen solution, and any alternatives you considered.
- Reference related issues (e.g., "Fixes #123").
- Ensure CI passes on all included checks before requesting review.

## Tests and Continuous Integration

Contributions that change behavior must include tests. Make sure new tests pass locally and in CI. If your change requires a change to CI configuration, explain why in the PR description.

## Code Style and Formatting

Follow existing project style. Where applicable, run the project's formatter and linters before committing. If the project includes configuration files for formatters or linters (e.g., .clang-format, .editorconfig), respect those settings.

## Security Issues

If you discover a security vulnerability, please do not open a public issue. Contact the maintainers privately using the email address in the repository (or via the security contact listed in the repository settings) so we can investigate and coordinate disclosure.

## License and Legal

By submitting a contribution you agree to the donation terms above. The project source code will continue to be available under GPL or a less restrictive license, at DarkGaze.Org's discretion, and contributor names will be maintained publicly.

If you have concerns about the donation terms, please raise them by opening an issue or contacting the maintainers before contributing significant work.

## Code of Conduct

By participating in this project you agree to abide by the project's Code of Conduct. Please see CODE_OF_CONDUCT.md for details. If a Code of Conduct file does not exist, follow common community standards of respect and professionalism.

## Thank You

Thank you for considering contributing to Gorgon. Your help makes this project better for everyone.

--
DarkGaze.Org
