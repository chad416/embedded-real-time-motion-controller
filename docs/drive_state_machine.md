# Drive State-Machine Design

The axis implements a CiA 402-style state machine with deterministic command acceptance and rejection.

## States

- Not Ready
- Switch On Disabled
- Ready To Switch On
- Switched On
- Operation Enabled
- Quick Stop Active
- Fault Reaction Active
- Fault

## Enable Path

```text
Not Ready
  -> Switch On Disabled
  -> Ready To Switch On
  -> Switched On
  -> Operation Enabled
```

The simulated enable command advances one deterministic state per control cycle, providing a traceable startup sequence.

## Motion Commands

| Command | Accepted In | Behavior |
| --- | --- | --- |
| `enable` | Non-faulted states | Advances CiA 402 enable path. |
| `disable` | Any state | Returns to Switch On Disabled and clears active mode. |
| `fault reset` | Fault | Returns to Switch On Disabled. |
| `quick stop` | Any state | Commands zero velocity and enters Quick Stop Active. |
| `move absolute` | Operation Enabled | Profile-position target. |
| `move relative` | Operation Enabled | Profile-position target relative to current model position. |
| `velocity` | Operation Enabled | Profile-velocity target. |
| `stop` | Any state | Commands zero velocity. |

Rejected commands are logged and force a command-rejected fault so unsafe command abuse is visible in validation evidence.
