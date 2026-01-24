# Validator Plugins

Drop `.ts` files into this folder to register custom validation rules.

Each module can export:
- `rule`: a single `ValidationRule`
- `rules`: an array of `ValidationRule`
- `default`: a `ValidationRule` or array of rules

Disable rules by writing a JSON array of rule IDs to
`localStorage["worldedit.validation.disabledRules"]`.
