# xd - Hexdump Utility

## Description

xd is a command-line utility that displays a hexdump table of specified files or standard input. It provides detailed information about the contents of a file in hexadecimal format, making it useful for analyzing binary data.

## Installation
1. Compile xd:
```sh 
make all # or <your_prefered_c_compiler> <your_c_flags> -o src/xd src/xd.c
```
2. Decide if your going to do a global instalation (with your root user) or a local installation.

### Local
1. Run:
```
make local-install
``` 
2. Done.
### Global
1. Ensure that your a root user with `id` or `who am i`. If not, use `su`.
2. Run:
```
make install
```
3. Done.

## Usage
```sh
xd [-D|-O][-L text][-K text][-R when][-bd][-c cols][-g size][-o file][-pu] [file...]
xd -i [-c columns][-n name][-o file][-u] [file...]
xd -?
```
For detailed usage informations, run (after instalation):
```sh
man xd
```

## Author
xd is created and maintained by Arthur de Souza Manske. For any inquiries or feedback, contact [usr.asm@pm.me](mailto:usr.asm@pm.me).

## License
1. You are able to distribute this program freely, provided you credit the author and distribute it within the source code.
2. Usage terms: No warranty; No involvement in illegal activities or pornographic content.
3. Contribution terms: Contributions must be made in good faith.
4. Reverse engineering is permitted under the same terms as the source code usage.
5. Derived source code must include these restrictions and may include additional ones.

If you disagree, refrain from using this software. By using it, you assume all associated risks, as the software comes with no warranty. Plus, don't use my name without permission.

## Contributing
Contributions to XD are welcome! Feel free to submit bug reports, feature requests, or pull requests through the GitHub repository. Bloating options will be rejected; functionality that can be replaced with pipes or other resources is considered unnecessary.
