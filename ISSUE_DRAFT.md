# Issue Checklist Draft

Use this file as a reference template when creating issues that include task checklists.
The checklist automation (`.github/workflows/subtasks-on-close.yml`) will enforce these rules when an issue is closed.

## How the Automation Works

When an issue is **closed**, the workflow will:

1. **Block closure** if any required (unmarked) checklist items are still unchecked — the issue is automatically reopened and a comment is posted listing the incomplete tasks.
2. **Auto-create subtasks** for any unchecked items marked with `[extra]` — a new issue is opened for each, and a summary comment is posted on the parent issue.
3. **Skip silently** for items marked `[optional]` or `[cancelled]` — these are treated as exempt and do not block closure or trigger subtask creation.

### Opting Out

Add the label **`no-subtask-automation`** to an issue to disable all checklist automation for that issue.

---

## Checklist Marker Reference

| Marker        | Behavior on close                                       |
|---------------|---------------------------------------------------------|
| *(none)*      | **Required** — blocks closure if unchecked              |
| `[optional]`  | Exempt — skipped, does not block closure                |
| `[cancelled]` | Exempt — skipped, does not block closure                |
| `[extra]`     | Auto-creates a new child issue if unchecked at closure  |

---

## Example Issue Checklist

```markdown
## Tasks

- [ ] Implement core feature
- [ ] Write unit tests
- [ ] Update documentation
- [ ] Code review approved
- [ ] Add integration tests [optional]
- [ ] Performance benchmarking [extra]
- [ ] Write migration guide [extra]
- [ ] Backport to v1 branch [cancelled]
```

In the example above:
- The first four items are **required** — the issue cannot be closed until they are checked.
- "Add integration tests" is **optional** — it is ignored by automation.
- "Performance benchmarking" and "Write migration guide" are **extra** — if left unchecked at closure, new issues will be created for each automatically.
- "Backport to v1 branch" is **cancelled** — it is ignored by automation.
