# Security Policy

## Supported versions

Only the current state of the `main` branch is maintained. The release tags bundle the ESPHome config, not a firmware image, and come without a support promise.

## Reporting a vulnerability

Please don't open a public issue. Report it privately through GitHub instead: **Security** tab →
**Report a vulnerability**. Reports are answered as time allows; this is a hobby project.

## Notes for users

The device is meant for a trusted home network:

- `secrets.yaml` holds the WiFi password, the web UI login and the API encryption key. Never commit it
  (it's git-ignored here).
- Don't expose the web UI, the ESPHome API or OTA updates to the internet.
- OTA updates have no password in this config. If other people use your network, add
  `password: !secret ota_password` under `ota:`.
- The web UI uses digest authentication over plain HTTP; the ESPHome API is encrypted.
