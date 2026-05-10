---
title: LGPL License
---

# LGPL license

Jett Graphics Library is distributed under the GNU Lesser General Public License, version 3.0 (LGPL-3.0). The authoritative license text is the top-level `LICENSE` file in the repository. This page summarizes the parts of LGPL-3.0 that matter most when you use, modify, or redistribute the library.

## What the license allows

- You can use the library in proprietary or open-source applications.
- You can redistribute the unmodified library.
- You can modify the library and redistribute your modified version.
- You can combine an application with the library, including through dynamic linking or another shared-library mechanism.

## What you must preserve when redistributing

- Keep the LGPL-3.0 notice with the library.
- Provide recipients with a copy of the GNU GPL and LGPL license texts when the license requires it.
- Preserve copyright notices and the no-warranty terms.
- If you distribute a modified version of the library itself, license those library changes under LGPL-3.0.

## Combined works and relinking

LGPL-3.0 is designed to let an application use a library without forcing the whole application onto the LGPL. The tradeoff is that recipients must still be able to replace or modify the LGPL-covered library portion.

In practical terms, if you distribute an application that links against Jett Graphics Library, you should use a linking and distribution approach that does not block a user from swapping in a modified version of the library for debugging or further development. A normal shared-library setup is the usual way to satisfy that requirement.

## If you modify the library

- Changes to the library itself remain under LGPL-3.0.
- You should make the modified library source available under the same license terms when you distribute it.
- Your application code that merely uses the library can stay under a different license, provided you still satisfy the LGPL conditions for the library portion.

## Header files and application code

LGPL-3.0 also covers some cases where application object code incorporates material from library header files. The license makes an explicit allowance for small macros, inline functions, and similar header-only fragments, while still requiring notice and license preservation when larger protected material is incorporated.

## No warranty

Like most free software licenses, LGPL-3.0 distributes the library without warranty. If you redistribute the library, keep that disclaimer intact.

## Read the full text

This page is a practical overview, not legal advice. For the exact legal terms, read the full LGPL-3.0 text in the repository `LICENSE` file.

