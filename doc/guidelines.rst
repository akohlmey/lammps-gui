**********************
Development Guidelines
**********************

Code Style
==========

The project follows these coding conventions:

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 100 characters
- **Formatting**: Enforced by ``.clang-format`` configuration (LLVM-based)
- **Comments**: Use Doxygen-style documentation comments
- **Naming**: 
  - Classes: CamelCase (e.g., ``CodeEditor``)
  - Functions: camelCase (e.g., ``reformatLine``)
  - Members: snake_case (e.g., ``reformat_on_return``)

Documentation
=============

All public classes and functions should have Doxygen documentation:

.. code-block:: cpp

   /**
    * @brief Brief description
    * 
    * Detailed description if needed
    * 
    * @param param1 Description of parameter
    * @return Description of return value
    */
   int myFunction(int param1);

Building
========

See :doc:`installation` for detailed build instructions. For development:

.. code-block:: bash

   # Debug build with Qt6
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \\
         -DLAMMPS_GUI_USE_PLUGIN=yes -DBUILD_DOC=no
   cmake --build build --parallel 2

Contributing
============

To contribute to LAMMPS-GUI:

1. Fork the repository on GitHub
2. Create a feature branch
3. Make your changes with proper documentation
4. Ensure code compiles with both Qt5 and Qt6 (if possible)
5. Test your changes thoroughly
6. Submit a pull request

All contributions must:

- Follow the existing code style
- Include Doxygen documentation for new public APIs
- Not break existing functionality
- Have GPG-signed commits
