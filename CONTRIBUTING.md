# Contributing

Pull requests are welcome here, for anything: fixes, other systems (Linux, for example), build changes, tools. The rules:

- The tpw-decomp submodule is never touched. Its pointer stays at the upstream commit.
- A changed game source file is a whole-file copy under overrides/ at the same path, with a row in FIXES.md: what the Mac
  does, what the port does and why.
- Host code for another system goes under platform/ and launcher/, behind that system's macro.
- Test aids stay out of a pull request.
- One concern per commit.
- Say in the pull request what you tested and on what: the edition and the system.

How a change lands: both public repositories are exported from one private working tree, so a pull request is not merged on
GitHub. An accepted change is re-landed in that tree (changes to game files as `#ifdef TPW_PORT_FIX` blocks, which the
export turns into the files under overrides/) and appears in the next export, with the contributor credited in the commit
message and in FIXES.md. The pull request is then closed.

tpw-decomp (https://github.com/ThemeParkWorldDecomp/tpw-decomp) is a pure translation of the Mac program and takes no pull requests and no changes. Anything
meant for it comes here.

Issues are welcome too. A useful bug report says which edition you play (US, European or Gold), attaches the launcher's log
and says what you clicked before it went wrong.
