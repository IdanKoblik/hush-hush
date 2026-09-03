/*
 * Command registry.
 *
 * COMMAND() in src/cmd/command.h drops one `const struct Command *` into its
 * own "commands.<name>" input section. Those names are not valid C identifiers,
 * so ld will not synthesise __start_/__stop_ symbols for them on its own -- this
 * script gathers them into a single .commands output section and brackets it.
 *
 * SORT_BY_NAME leaves the table sorted by section name, which is the command
 * name, so find_command() can bsearch it.
 *
 * The table holds pointers, so it needs relocations: .data.rel.ro is where a PIE
 * puts read-only-after-relocation data. Placing it in .rodata forces a DT_TEXTREL.
 */
SECTIONS
{
    .commands : ALIGN(8)
    {
        __start_commands = .;
        KEEP(*(SORT_BY_NAME(commands.*)))
        __stop_commands = .;
    }
}
INSERT AFTER .data.rel.ro;
