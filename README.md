# herofand

Minimal C daemon for the Hero fan-control use case.

## Goals

- minimal code and dependencies
- strict build settings
- predictable systemd deployment
- responsive polling-based control for the exact Hero host policy

## Target toolchain

This project is intentionally tuned for `hero.makeitwork.cloud`:

- GCC 11.3.x
- GNU Make 4.3
- Python 3.11.x for lightweight repo checks
- C17 source standard

The repo avoids a clang-based workflow because that toolchain is not installed on the target host.

## Layout

- `src/` - production sources
- `include/` - public/internal headers shared across translation units
- `tests/` - lightweight smoke tests
- `scripts/` - helper scripts used by local tooling and hooks

## Build

```sh
make
```

## Test

```sh
make test
```

## Install

Install the binary and systemd unit into the target filesystem:

```sh
make install
```

Override destinations when staging a package or host image:

```sh
make install DESTDIR=/tmp/herofand-root PREFIX=/usr/local UNITDIR=/etc/systemd/system
```

## Systemd

The repo ships `systemd/herofand.service`.

Expected deployment flow on the target host:

```sh
sudo make install
sudo systemctl daemon-reload
sudo systemctl enable --now herofand.service
```

Do not enable it alongside the existing shell-based `hero-fand.service`; only one controller should own the fan outputs at a time.

## Pre-commit

Install hooks:

```sh
pre-commit install
```

Run all hooks:

```sh
pre-commit run --all-files
```

## CI and release artifacts

GitHub Actions is configured to target a RHEL 9-compatible userspace:

- `CI` runs build, test, and pre-commit checks in a Rocky Linux 9 container.
- `Release Please` watches conventional commits on `master`/`main`, opens a release PR, updates `VERSION`, updates `CHANGELOG.md`, and creates tags/releases.
- `Release` builds a tarball artifact containing the `herofand` binary and the systemd unit.

Release tags are expected to match `VERSION` and use the form `vX.Y.Z`.

The workflow publishes release artifacts instead of committing binaries back into the repository.

## Automated versioning

Version increments are automated through `release-please`.

- Write commits using the conventional commit format.
- Merge them to `master`/`main`.
- `release-please` opens or updates a release PR with the next version.
- Merging that PR updates `VERSION`, updates `CHANGELOG.md`, and creates the GitHub tag/release.
- The release workflow then builds and uploads the RHEL 9-compatible tarball artifact.

`VERSION` remains in the repo, but you should not bump it manually for normal releases.

## Status

This is an implementation-ready project with a working daemon core, build/test flow, and installable systemd unit.

## Versioning

The project uses semantic versioning, starting from the value in `VERSION`.
