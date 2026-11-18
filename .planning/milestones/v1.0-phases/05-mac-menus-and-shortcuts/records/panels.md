# Native file panels (plan 05-10)

Live measurement of the file panels that the application opens, taken on 2026-11-15 with the harness `mac_check` (section `panels`). It answers the live half of MAC-11: is each reachable file panel, including the two relinking panels changed in plan 05-09, the native macOS panel, does it open in a folder below the harness output directory, and does cancelling it return an empty path and leave the relinking substitution unchanged (UI-SPEC E12)?

- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, keyboard layout `com.apple.keylayout.German`.
- Command (from `{repo}`, run by `build/scratch/run-m14.sh`, which adds a 200 s kill watchdog): `D=build/scratch/mac/m14a; mkdir -p "$D/home"; env HOME="$PWD/$D/home" CFFIXED_USER_HOME="$PWD/$D/home" build/macos-release/mac_check "$D" --section dialogs,panels -ApplePersistenceIgnoreState YES`. The run was made once, with the committed harness (commit `efe9cbdb`), into `m14a`; its `summary.txt` was copied to `build/scratch/mac/m14/` where the plan's check reads it. No second run was made. The same run produced `dialogs.md`.
- The home directory was redirected below the output directory (`home_redirected: yes`), so panel state and folder listings stayed below `{repo}/build/`. Every panel started in a folder below the output directory (`panels/`, `panels/input`, `panels/output`, `relink/old`) and was cancelled by the harness from a block that runs inside the panel's modal session; nothing was navigated or selected.
- No key or click was physically pressed: the harness only cancelled each panel with `[NSSavePanel cancel:]` from inside its own process and posted no event to any other application; the pointer was not moved. There was no sign of user interference (the application was active for the whole section and each panel was seen).

Verdict rule of `panels_verdict` (PASS when all hold): the application is active; for all five rows the modal window is an `NSSavePanel` kind (`native: yes`), its directory lies below the output directory, `cancel_result` is `empty` (the `QFileDialog` object that the static function created finished with `QDialog::Rejected` and emitted no `filesSelected`, so the function returned an empty string), and for both relinking rows the relinker's substitution of a collected path is unchanged after the dialog's own `commitChanges` slot ran.

## Raw lines

```
panels_app_active: yes
panels_relink_file_class: NSOpenPanel
panels_relink_file_native: yes
panels_relink_file_directory: {out}/relink/old
panels_relink_file_directory_under_out: yes
panels_relink_file_title: Substitution File for {out}/relink/old/page.png
panels_relink_file_cancel_result: empty
panels_relink_file_substitution_unchanged: yes
panels_relink_folder_class: NSOpenPanel
panels_relink_folder_native: yes
panels_relink_folder_directory: {out}/relink/old
panels_relink_folder_directory_under_out: yes
panels_relink_folder_title: Substitution Directory for {out}/relink/old
panels_relink_folder_cancel_result: empty
panels_relink_folder_substitution_unchanged: yes
panels_open_project_class: NSOpenPanel
panels_open_project_native: yes
panels_open_project_directory: {out}/panels
panels_open_project_directory_under_out: yes
panels_open_project_title: Open Project
panels_open_project_cancel_result: empty
panels_input_folder_class: NSOpenPanel
panels_input_folder_native: yes
panels_input_folder_directory: {out}/panels/input
panels_input_folder_directory_under_out: yes
panels_input_folder_title: Input Directory
panels_input_folder_cancel_result: empty
panels_output_folder_class: NSOpenPanel
panels_output_folder_native: yes
panels_output_folder_directory: {out}/panels/output
panels_output_folder_directory_under_out: yes
panels_output_folder_title: Output Directory
panels_output_folder_cancel_result: empty
panels_not_opened: save_project_as, out_of_memory_save, files_to_insert (need a loaded project; plain QFileDialog calls without options, checked by testNoDontUseNativeDialog)
panels_verdict: PASS
```

`{out}` stands for `{repo}/build/scratch/mac/m14a`.

## Table

| Panel (row) | Class | Native | Start directory | Title (panel property) | Cancel result | Substitution unchanged |
|-------------|-------|--------|-----------------|------------------------|---------------|------------------------|
| Relinking, file (`relink_file`) | NSOpenPanel | yes | `{out}/relink/old` | Substitution File for `{out}/relink/old/page.png` | empty | yes |
| Relinking, folder (`relink_folder`) | NSOpenPanel | yes | `{out}/relink/old` | Substitution Directory for `{out}/relink/old` | empty | yes |
| Open Project (`open_project`) | NSOpenPanel | yes | `{out}/panels` | Open Project | empty | not applicable |
| New project, input folder (`input_folder`) | NSOpenPanel | yes | `{out}/panels/input` | Input Directory | empty | not applicable |
| New project, output folder (`output_folder`) | NSOpenPanel | yes | `{out}/panels/output` | Output Directory | empty | not applicable |

Only one native panel was open at a time: each row's call returned before the next began (the section is sequential and the panels are application-modal).

## Panels not opened

`Save Project As` (`MainWindow.cpp:1427`), the save panel of `OutOfMemoryDialog` (`OutOfMemoryDialog.cpp:62`) and `Files to insert` (`MainWindow.cpp:1862`) need a loaded project. They are plain `QFileDialog` static calls without options, and `TestMacQt::testNoDontUseNativeDialog` (plan 05-09) scans the sources and finds no option that would force a non-native panel, so they are native by the same mechanism that the five measured rows show.

## Observations

- Native folder panel: the folder panels (`relink_folder`, `input_folder`, `output_folder`) are an `NSOpenPanel` in directory-choosing mode, which shows files greyed out. This is accepted: the user picks a folder there (UI-SPEC E12, partial). The greying itself was not photographed; it is the documented behaviour of that panel mode.
- Relinking title: the panel object carries the titles `Substitution File for <path>` and `Substitution Directory for <path>` (`panels_relink_*_title`, read from the panel's title and message). Whether macOS draws that text in the panel window was not observed, because no screenshot was taken (screen capture would need a privacy permission, which the phase excludes, D-12). The paths in the titles are the harness paths below the output directory, with no long text to wrap; the long-path case (UI-SPEC E12 long-text) is therefore not measured.
- Cancel: all five panels returned an empty path when cancelled, and both relinking rows left `relinker()->substitutionPathFor` equal to the original normalized path after the dialog's `commitChanges` slot ran.
- `Files to insert` (`MainWindow.cpp:1862`) is already native at HEAD. Its `ProxyModel`, which would hide files already in the project, is not consulted by a native panel. This predates the phase and is not repaired here (RESEARCH Pitfall 9); it stays on the list for the closing report.
