# NSAuthenticator

An authentication system for the Switch

## Why an authentication system?

The authentication is sometimes necessary to protect its content from other gamers and particularly when using [NSParentalControl](https://github.com/TristanIsrael/NSParentalControl) to avoid other gamers to use others play time.

## Current version

⚠️ This project is currently under development. There is no official release.

## Features

- When a gamer starts a game he chooses the profile (user) he wants to play with
- The first time a profile is used, the authenticator window is shown and asks the user to **create** its personal code.
- The next time the profile is used, the authenticator window is show and asks the user to **verify** its personal code.
  - If the code entered is wrong the game is stopped.

## Credits

- Niels Lohmann <https://nlohmann.me> for JSON C++ library
- @SciresM for Atmosphère
- Sean Barrett for STB Truetype