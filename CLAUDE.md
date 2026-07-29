# ESP32 Examples

## Git commits

Never commit changes without an explicit instruction to do so. Always wait for user approval before running git commit.

## Creating new projects from template

When creating a new project from the `template` directory:

1. Copy the template directory to a new project directory
2. Add the new project to `esp-examples.code-workspace`
3. **Rename all environments in `platformio.ini`** -- replace `template-` prefix with your project name (e.g., `template-esp32-s3-super-mini` becomes `basic-cli-esp32-s3-super-mini`)
4. Update the `default_envs` line to reference the new environment names
5. Update the project's `CLAUDE.md` with project-specific documentation
