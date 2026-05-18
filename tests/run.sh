#!/usr/bin/env sh
# Smoke tests. Run via `make test` (which builds with ASan first).
set -eu

BIN="${BIN:-./zenv}"
fail=0

check() {
	desc=$1; shift
	if "$@" >/dev/null 2>&1; then
		printf '  ok   %s\n' "$desc"
	else
		printf '  FAIL %s\n' "$desc"
		fail=$((fail + 1))
	fi
}

check_fail() {
	desc=$1; shift
	if "$@" >/dev/null 2>&1; then
		printf '  FAIL %s (expected nonzero)\n' "$desc"
		fail=$((fail + 1))
	else
		printf '  ok   %s\n' "$desc"
	fi
}

check_eq() {
	desc=$1; expected=$2; shift 2
	actual=$("$@" 2>/dev/null || true)
	if [ "$actual" = "$expected" ]; then
		printf '  ok   %s\n' "$desc"
	else
		printf '  FAIL %s (got: %s)\n' "$desc" "$actual"
		fail=$((fail + 1))
	fi
}

echo "== smoke =="
check     "version exits 0"               "$BIN" version
check     "help exits 0"                  "$BIN" help
check     "shell-init zsh exits 0"        "$BIN" shell-init zsh
check_fail "unknown command fails"        "$BIN" nope
check_fail "shell-init bash unsupported"  "$BIN" shell-init bash

# End-to-end: set a var, eval shell-init in a fresh zsh, verify export
# round-trips correctly — including a value with shell metacharacters.
if command -v zsh >/dev/null 2>&1; then
	tmpstate=$(mktemp -d)
	export XDG_STATE_HOME="$tmpstate"

	eval "$("$BIN" var set CHECK_IP '10.10.14.129')" >/dev/null 2>&1
	eval "$("$BIN" var set EVIL "rm -rf ~; echo pwned")" >/dev/null 2>&1

	check "shell-init exports persistent var" \
	    zsh -c "eval \"\$($BIN shell-init zsh)\"; [ \"\$CHECK_IP\" = 10.10.14.129 ]"
	check "shell-init quotes evil values" \
	    zsh -c "eval \"\$($BIN shell-init zsh)\"; [ \"\$EVIL\" = 'rm -rf ~; echo pwned' ]"
	check_eq "var get returns the value" "10.10.14.129" "$BIN" var get CHECK_IP
	check    "var unset removes it" sh -c "$BIN var unset CHECK_IP && ! $BIN var get CHECK_IP"

	# reload: simulate a long-running shell that loaded vars, then another
	# shell mutated vars.toml behind its back.
	check "reload picks up new vars and unsets removed ones" zsh -c "
		eval \"\$($BIN shell-init zsh)\"
		$BIN var unset EVIL >/dev/null
		$BIN var set NEW value >/dev/null
		eval \"\$($BIN reload)\"
		[ -z \"\$EVIL\" ] && [ \"\$NEW\" = value ]
	"

	# cd hook: starting_path should cd the shell on init.
	cd_target=$(mktemp -d)
	"$BIN" var set starting_path "$cd_target" >/dev/null
	# zsh increments SHLVL on launch, so SHLVL=0 in env gives SHLVL=1 inside.
	check "shell-init cds to \$starting_path on SHLVL=1" env SHLVL=0 zsh -c "
		eval \"\$($BIN shell-init zsh)\"
		[ \"\$PWD\" = \"$cd_target\" ]
	"
	"$BIN" var unset starting_path >/dev/null
	rmdir "$cd_target"

	rm -rf "$tmpstate"
	unset XDG_STATE_HOME
fi

if [ "$fail" -gt 0 ]; then
	echo "FAILED: $fail"
	exit 1
fi
echo "all green"
