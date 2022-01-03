# pgts
store your time series in PostgreSQL as a professional.

```
running encode [u8 / normal]      ... OK
running decode [u8 / normal]      ... OK
running encode [u8 / signbit]     ... OK
running decode [u8 / signbit]     ... OK
running encode [u8 / overflow]    ... OK
running decode [u8 / overflow]    ... OK
running bench  [u8 / rand]        ... [decode 155.92GiB/s encode 112.96GiB/s | 781.25MiB -> 854.50MiB]
running bench  [u8 / ordered]     ... [decode 1482.96GiB/s encode 1027.67GiB/s | 781.25MiB -> 12.30MiB]
```
