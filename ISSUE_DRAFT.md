# Implement async-capable resource locating function with configurable search order and HTTP integration

## Summary
Create a function responsible for locating resources required by an application.

## Checklist (main)
- [ ] Search executable location for resource
- [ ] Search current working directory
- [ ] Search standard locations (e.g., user/app data folders)
- [ ] Provide API to set search order
- [ ] Provide API to override search locations
- [ ] Integrate with HTTP module to download missing resources and cache them
- [ ] Provide asynchronous API for resource location

## Extended Tasks (not required for acceptance) — mark these with [extra]
- [ ] UI for download (requires network::http support for progress) [extra]
- [ ] If resource is found compressed, uncompress it (lzma) [extra]
- [ ] Update check from the internet (requires hashing) and download if the resource is updated [extra]

## Automation opt-out
If you want to disable the automation for a particular issue, add the label `no-subtask-automation` to the issue; the workflow will detect this label and skip all checks and auto-creation.

## Notes
- Design should be extensible to support new locations or protocols.
- Ensure a clear configuration interface for developers to modify order and override.
- Download logic should trigger only if file is missing after all local searches.
- Make sure download and caching work seamlessly with the locating logic.
- Both sync and async methods should be provided where possible.

## Acceptance Criteria
- API for setting and getting search order is available.
- Option to override default search locations.
- When resource is not found locally, and HTTP info is provided, file is downloaded and cached.
- Unit tests validate all major behaviors.
- Asynchronous resource location is supported and tested.

---
**Labels**: `enhancement`, `modules`, `OS`, `Filesystem`, `Networking`
**Type**: `Feature`
