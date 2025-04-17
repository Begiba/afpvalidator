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

# Contributing
Your contributions are highly valued and welcomed! Whether you want to add new features, fix bugs, improve documentation, or simply enhance the overall quality of the tool, your efforts are appreciated. Fork the repository, make your improvements, and submit a pull request. Thank you for helping make AfpValidator better for everyone!

# License
This tool is licensed under the MIT License - see the [LICENSE file](/LICENSE) for details.

#Enjoy AfpValidator!
[Began BALAKRISHNAN](http://www.linkedin.com/in/began-balakrishnan-0a20b221) (Author)
