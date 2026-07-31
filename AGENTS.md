# AGENTS.md

Safety and contribution guidance for the Hero fan-control daemon.

## Repository role

`herofand` is a minimal C daemon and systemd unit tuned for the `hero` host. Source lives in `src/` and `include/`; `tests/` contains local smoke tests; `systemd/herofand.service` is installed with the binary.

## Safe development

Use the narrowest relevant local check:

```bash
make test
make format-check
make dev-check
pre-commit run --all-files
```

`make format` rewrites C sources. Recheck the diff after running it. Some `dev-check` tools are Fedora development dependencies and are not required on the RHEL-compatible runtime host.

## Hardware and service boundaries

The production daemon writes fan-control outputs. Do not run it against host hardware during ordinary validation.

- `make install` writes to the configured prefix and systemd unit directory. Use `DESTDIR` for an authorized staging test.
- `sudo make install`, `systemctl daemon-reload`, and service enable/start/stop/restart actions require explicit confirmation.
- Never enable `herofand.service` alongside the legacy `hero-fand.service`; only one controller may own the fan outputs.
- Preserve the exact Hero policy unless the user explicitly requests a behavior change and the hardware assumptions are revalidated.

## CI and releases

`main` is protected and requires the RHEL 9-compatible build, test, and lint check. Work on a feature branch and use Conventional Commits. Release Please owns routine `VERSION`, changelog, tag, and release updates; do not bump `VERSION` manually for a normal release.

Run local checks before requesting a pull request, but do not install, deploy, or operate the target service as part of validation.
