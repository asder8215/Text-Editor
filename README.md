# App Name: TeXiT

This is a simple text editor built in C, utilizing GTK4, Libadwaita, and Blueprint Compiler packages. The main motivation behind the text editor is purely educational. We wanted to build an editor that, at the end, involves live editing with multiple individuals. To do it in C was an extra challenge for us. Although live editing is not implemented into our application as of now, we still intend to develop that into our application.

# Demo of App

![Demo of App](https://github.com/asder8215/Text-Editor/blob/Mahdi/readme_imgs/Demo%201.gif)
![Share Message Dialog](https://github.com/asder8215/Text-Editor/blob/Mahdi/readme_imgs/Share%20Message%20Dialog.png)

<img src="https://github.com/asder8215/Text-Editor/blob/Mahdi/readme_imgs/Demo%201.gif" width="50%" height="50%" text-align="center" display="block">
<img src="https://github.com/asder8215/Text-Editor/blob/Mahdi/readme_imgs/Share%20Message%20Dialog.png" width = "60%" height="60%" text-align="center" display="block">

So far our app contains:
- New File, Open File, and Save File buttons.
- Tabs to allow for more text files on the screen.
- Recognition of unsaved content on the tab.
- Share Message Dialog (no functionality yet)

In the future we plan to add:
- Setting Dialog Screen with more customizability options (font family, font size, code syntax coloring, etc.)
- Hosting files on a server for multiple people to edit the same file and view changes live.
- Other miscellaneous additions to the app (word count, spellcheck, etc.).

# Install

Run the [install script](./install): `./install sys` to install it for all users (`/usr/local/`), or `./install user` for just your user (`~/.local/`). To uninstall the application on `sys` or `user`, run `./install sys uninstall` or `./install user uninstall` respectively. Make sure to have all the [build dependencies](#build-dependencies) installed.

# Build Dependencies
 - ## Blueprint
    - Builds `.blp` files into `.ui` files that can be used by GtkBuilder. Install it [here](https://jwestman.pages.gitlab.gnome.org/blueprint-compiler/setup.html)


# Language Server (IDE)

If using `neovim` or `vscode`, clangd will need the file `compile_commands.json` to find gtk4 libraries.
To generate the file install [`bear`](https://github.com/rizsotto/bear) from your distro's package manager and run `bear -- make`.
