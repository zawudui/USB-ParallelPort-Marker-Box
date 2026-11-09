# Upload the clean personal source snapshot

This directory is the curated personal source release for **Li Jingwei**. It contains no `.git` directory or private commit history. Original material is credited to Li Jingwei following the owner's explicit confirmation on 2026-11-09; existing third-party notices remain applicable.

## New repository

Create a new empty repository under your personal GitHub account. The existing `zawudui/usb-db25-marker-box` name is occupied by the private engineering repository, so choose a distinct new name. Start without an automatically created README or licence. Keep the new repository private while checking the initial upload.

Unpack the supplied source ZIP, open a terminal in its top-level folder and run:

```sh
python tools/release_audit.py
git init -b main
git add .
git status --short
git commit -m "Initial public source snapshot: USB-DB25 Marker Box r7.2"
git remote add origin https://github.com/zawudui/USB-ParallelPort-Marker-Box.git
git push -u origin main
```

The checked-in `.gitattributes` preserves exact file bytes across operating systems. The `.gitignore` explicitly includes both fabrication and print ZIP assets; no force-add is necessary. Set your Git author name/email (or GitHub noreply address) before committing.

## Verify the uploaded files

Confirm that both workflows pass: source snapshot integrity on Windows/Linux, and firmware host tests/target build. Download a fresh ZIP of the new repository and run `python tools/release_audit.py` again. The authoritative root `SOURCE_SHA256SUMS` lists all public files except itself.

The original engineering and organization repositories remain private and unchanged. Do not fork, mirror or import their histories. The new repository should start with the single reviewed snapshot commit. See [release checks](RELEASE_GATE.md) for remaining prototype limits before making the new repository public.
