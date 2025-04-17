# AfpValidator
This tool validates AFP/MO:DCA files according to the specification.  
It analyzes the document structure, identifies errors, and provides statistics.  

This is my attempt to better understand the structure of AFP files.

## AFP Standards
IPDS and MO:DCA are the heart of the AFP Architecture.  
AFP Consortium publish the standards and reference documents.  
For more details, please refer to these new references on the [AFP Consortium website](https://www.afpconsortium.org/publications.html).

# Getting Started
## Prerequisites
Ensure you have a C compiler installed on your system. If you don't have one, you can use [GCC](https://gcc.gnu.org/) for most platforms.

## Cloning the Repository
```
git clone https://github.com/begiba/afpvalidator.git
cd afpvalidator
```

## Build
```
$ gcc -o AfpValidator ./afpvalidator.c 
```

# Usage
```
Usage: AfpValidator <afp_file> [-v]  
  -v: Verbose mode (print details of each structured field)
```
> [!WARNING]
> The length of the output is big in verbose mode.  It will be difficult to view and analyze in console.

> [!TIP]
> Redirect the output to a text file for easy view and analyze.

# Output
```
            __   __      __   _ _     _       _
     /\    / _|  \ \    / /  | (_)   | |     | |
    /  \  | |_ _ _\ \  / /_ _| |_  __| | __ _| |_ ___  _ __
   / /\ \ |  _| '_ \ \/ / _` | | |/ _` |/ _` | __/ _ \| '__|
  / ____ \| | | |_) \  / (_| | | | (_| | (_| | || (_) | |
 /_/    \_\_| | .__/ \/ \__,_|_|_|\__,_|\__,_|\__\___/|_|
              | |
              |_|                      By Began BALAKRISHNAN


Analyzing AFP file: .\sampleafp\x2.afp (Size: 67347 bytes)


AFP File Analysis Summary:
-------------------------
Total structured fields: 35
Errors detected: 0
Begin Document found: Yes
End Document found: Yes

AFP Structure Summary:
---------------------
Document structure is properly nested and complete.

Content Summary:
  - Pages: 1
  - Objects: 1
  - Resources: 1

AFP Content Statistics:
----------------------
Documents:         1
Page Groups:       0
Pages:             2
Overlays:          0
Resource Groups:   0
Presentation Text: 0
Images:            0
Graphics:          0
Barcodes:          0
Fonts:             0
Form Definitions:  0
Page Segments:     0

Validation result: VALID
```

# Contributing
Your contributions are highly valued and welcomed! Whether you want to add new features, fix bugs, improve documentation, or simply enhance the overall quality of the tool, your efforts are appreciated. Fork the repository, make your improvements, and submit a pull request. Thank you for helping make AfpValidator better for everyone!

# License
This tool is licensed under the MIT License - see the [LICENSE file](/LICENSE) for details.

#Enjoy AfpValidator!
[Began BALAKRISHNAN](http://www.linkedin.com/in/began-balakrishnan-0a20b221) (Author)
