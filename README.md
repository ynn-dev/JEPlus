# JEPlus

A mod that adds NVIDIA DLSS Frame Generation and NVIDIA Reflex to *Judgment* (2018).

> **Status:** early development

## Building

Requires MSVC and uv

```powershell
uv run scripts\package.py
```

The result can be found in `build\JEPlus.zip`.

## Credits

JEPlus is built with these open-source libraries:

- [spdlog](https://github.com/gabime/spdlog)
- [Glaze](https://github.com/stephenberry/glaze)
- [SafetyHook](https://github.com/cursey/safetyhook)
- [NVIDIA Streamline](https://github.com/NVIDIA-RTX/Streamline)

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for their licenses and the NVIDIA runtime files.

## License

JEPlus is released under the [MIT License](LICENSE).
