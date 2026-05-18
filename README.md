# zenv

Persistent shell variables for zsh, written in C.

Stores `key = value` pairs in `~/.local/state/zenv/vars.toml` and exports
them into every new shell. Replaces ad-hoc state in `~/.zshrc` and
sourced `*.sh` files.

## Build

```sh
./scripts/vendor-tomlc99.sh
make
sudo make install
```

`make install` puts the binary in `/usr/local/bin/zenv`. Pass
`PREFIX=/usr` to match the AUR layout.

## Use

```sh
# ~/.zshrc
eval "$(zenv shell-init zsh)"
```

Then:

```sh
zenv var set IP 10.10.14.129
zenv var ls
zenv var get IP
zenv var unset IP
```

Values survive across shells. Open a new terminal and `$IP` is already
set.

## Deps

Build-time: C11 compiler, make. Runtime: libc only. TOML parser is
vendored under `third_party/`.

## License

MIT.
