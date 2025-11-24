# Mud98 Glossary

- **Area**: A set of rooms, mobs, and objects loaded from `area/` files; may be single- or multi-instance.
- **Descriptor**: A network connection (telnet/TLS) representing a connected client before/after login.
- **Mobile (mob)**: A non-player character instance derived from a mob prototype.
- **Prototype**: Template data for mobs/objects/rooms; instances are created from prototypes at runtime.
- **OLC**: Online Creation tooling for editing game data in-session (e.g., `cedit`, `qedit`).
- **MTH**: MUD Telnet Handler; implements telnet options (GMCP/MCCP/MSDP/MSSP).
- **MCCP**: MUD Client Compression Protocol (compression layer).
- **MSSP/MSDP**: Server status/data protocols for clients.
- **GMCP**: Generic MUD Communication Protocol (structured client-server messages).
- **Descriptor state**: Connection/login state machine for a descriptor.
- **Affect**: Timed/stackable status applied to entities.
- **Quest**: Scripted tasks tightly integrated with Mob Progs and OLC.
- **Mud98 config**: Runtime settings in `mud98.cfg`; read when running from the repo root.
