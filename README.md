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

### Sync across open shells

After mutating vars in one terminal, sync any other already-open shell
with:

```sh
zenv reload
```

It diffs against `$_ZENV_LOADED` (set by `shell-init`) and emits the
delta — unsets what was removed, exports what's new.

### Starting directory

If you set the special variable `starting_path`, new shells will `cd`
to it on startup (guarded by `SHLVL=1`, so nested shells aren't
affected):

```sh
zenv var set starting_path "$PWD"
```

## Deps

Build-time: C11 compiler, make. Runtime: libc only. TOML parser is
vendored under `third_party/`.

## License

MIT.
