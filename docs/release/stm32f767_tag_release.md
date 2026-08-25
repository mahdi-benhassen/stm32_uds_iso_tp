# STM32F767 tagged release and hardware delivery

The workflow [`stm32f767-release.yml`](../../.github/workflows/stm32f767-release.yml) runs when a tag matching `v*` is pushed. It checks out the exact tag, builds the STM32F767 CubeMX application with the repository’s GNU Arm toolchain, and publishes a GitHub Release containing versioned firmware assets.

## Published assets

Each tagged release contains the following files under the `stm32f767_uds_iso_tp-<tag>` naming prefix:

| Asset | Purpose |
|---|---|
| `.bin` | Raw image for STM32 programming tools at `0x08000000` |
| `.hex` | Intel HEX image for programming tools that accept HEX |
| `.elf` | Debug symbols and post-build inspection |
| `.map` | Link map for memory and section review |
| `.size.txt` | `arm-none-eabi-size` summary |
| `.manifest.txt` | Tag, commit, target, flash address, and security-boundary metadata |
| `SHA256SUMS` | SHA-256 checksums for every release asset |

The release job uses the GitHub-provided workflow token only to publish assets for the tag. It does not add production secrets or a SecurityAccess algorithm to the firmware.

## Creating a release

After merging the desired source state, push an annotated or lightweight version tag from a trusted development environment:

```sh
git tag v1.1.0
git push origin v1.1.0
```

The workflow builds the tag itself, so the release binaries correspond to the exact tagged commit rather than to a later branch state. Review the generated manifest, size report, map file, and checksums before programming a board.

## Optional physical STM32 flash

A GitHub-hosted runner cannot access a user’s STM32 board. The workflow therefore keeps physical programming disabled by default and defines a separate, opt-in `flash-hardware` job for a repository-owned or organization-owned self-hosted runner. The runner must be labeled `self-hosted`, `stm32f767`, and `flash`, and must have the STM32CubeProgrammer CLI installed at `/usr/local/bin/STM32_Programmer_CLI` or at the path supplied by the `STM32_PROGRAMMER_CLI` repository variable.

To enable the job, configure all of the following in the repository settings:

| Setting | Required value or purpose |
|---|---|
| Repository variable `STM32F767_TAG_FLASH_ENABLED` | Exact string `true` |
| Protected environment `stm32f767-flash` | Required reviewers and any organization hardware controls |
| Environment secret `STM32F767_FLASH_CONFIRMATION` | Exact string `I_UNDERSTAND_STM32F767_FLASH` |
| Optional repository variable `STM32_PROGRAMMER_CLI` | Absolute CLI path on the self-hosted runner |
| Optional repository variable `STM32_F767_FLASH_ADDRESS` | Defaults to `0x08000000`; change only with a reviewed linker/application layout |

The flash job downloads the exact artifact produced by the build job, verifies `SHA256SUMS`, writes the `.bin` over SWD, verifies the write, and resets the target. The environment approval is a deliberate human checkpoint. Do not enable it unless the connected board, debug probe, power state, target identity, and recovery procedure have been verified.

The job is intentionally not available on the ordinary Ubuntu-hosted runner. Without the repository variable, a tag creates the GitHub Release but performs no physical programming. This distinction prevents an accidental release from being treated as proof that a board was flashed or that application-level diagnostics passed.

## Validation boundary

The existing CI workflow continues to run host tests, sanitizers, portability checks, static analysis, the STM32F767 cross-build, coverage, and HIL dry-run report generation. The tagged release workflow adds artifact production and checksum verification. It does not replace physical CAN validation, SWD probe validation, board-specific power/recovery checks, or post-flash UDS smoke tests.

The STM32F767 application remains a Classical CAN/bxCAN target. The `.bin` and `.hex` images must not be used for an unrelated MCU, linker layout, bootloader offset, or CAN-FD target without a separate reviewed build configuration.

## References

[1]: https://docs.github.com/en/actions/using-workflows/events-that-trigger-workflows "GitHub Actions workflow events"
[2]: https://docs.github.com/en/actions/deployment/targeting-different-environments/using-environments-for-deployment "GitHub Actions environments and approvals"
[3]: https://www.st.com/en/development-tools/stm32cubeprog.html "STM32CubeProgrammer"
