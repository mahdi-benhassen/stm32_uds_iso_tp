# UDS Services

The UDS server uses compile-time service gates and callback contracts. Unsupported services must not return fake success. When the response buffer is sufficient, the dispatcher returns a negative response with an appropriate NRC; the public NRC reference describes common meanings and values [1].

| SID | Service | Reference status |
|---:|---|---|
| `0x10` | DiagnosticSessionControl | Supported for default, programming, and extended sessions. |
| `0x11` | ECUReset | Supported through a policy callback; reset is deferred to mainline. |
| `0x19` | ReadDTCInformation | Supported through a bounded DTC callback. |
| `0x22` | ReadDataByIdentifier | Supported through the table-driven registry. |
| `0x27` | SecurityAccess | Supported through the replaceable provider; checked-in provider is non-production. |
| `0x28` | CommunicationControl | Callback exists; default reference policy returns unsupported. |
| `0x2E` | WriteDataByIdentifier | Registry-controlled and read-only by default. |
| `0x2F` | InputOutputControlByIdentifier | Disabled by default. |
| `0x31` | RoutineControl | Callback exists; default reference routines return unsupported. |
| `0x34` | RequestDownload | Bounded interface only; Flash callbacks are not installed by default. |
| `0x36` | TransferData | Requires an active accepted download and valid block sequence. |
| `0x37` | RequestTransferExit | Bounded finish/CRC callback; activation remains product/bootloader-owned. |
| `0x3E` | TesterPresent | Supported with optional positive-response suppression. |
| `0x85` | ControlDTCSetting | Callback-controlled enable/disable state. |

Important NRCs include `0x11` ServiceNotSupported, `0x12` SubFunctionNotSupported, `0x13` IncorrectMessageLengthOrInvalidFormat, `0x14` ResponseTooLong, `0x22` ConditionsNotCorrect, `0x24` RequestSequenceError, `0x31` RequestOutOfRange, `0x33` SecurityAccessDenied, `0x35` InvalidKey, `0x36` ExceedNumberOfAttempts, `0x37` RequiredTimeDelayNotExpired, `0x70` UploadDownloadNotAccepted, `0x71` TransferDataSuspended, `0x72` GeneralProgrammingFailure, `0x73` WrongBlockSequenceCounter, and `0x78` ResponsePending [1].

## References

[1]: https://uds.readthedocs.io/en/v0.1.0/autoapi/uds/messages/nrc/index.html "UDS NRC public reference documentation"
