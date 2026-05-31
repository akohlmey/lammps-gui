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
  - Classes: CamelCase (e.g., ``CodeEditor``, ``ChartWindow``)
  - Qt overrides: camelCase (e.g., ``setFont``, ``eventFilter``)
  - Custom methods/slots: camelCase (e.g., ``stopRun``, ``saveAs``)
  - Members: camelCase (e.g., ``reformatOnReturn``)

Documentation
=============

Added functionality should be documented both in the User's Guide and
the Programmer's Guide section of the documentation.  The documentation
should have suitable ``.. index::`` directives to populate the index
with suitable keywords.  The documentation is written in
`reStructuredText
<https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html>`_
and imports documentation of the source code from `Doxygen
<https://doxygen.nl/>`_ using the `Breathe Sphinx plugin
<https://breathe-doc.org>`_.  The documentation should translate cleanly to
`HTML <https://en.wikipedia.org/wiki/HTML>`_ and `PDF
<https://en.wikipedia.org/wiki/PDF>`_ using the ``html`` or ``pdf``
build targets.  Additionally the build targets ``spelling`` and
``linkcheck`` are available to run a spell checker on the documentation
and validate external links.

All public classes and functions should have Doxygen documentation as
demonstrated below:

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
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
         -DLAMMPS_GUI_USE_PLUGIN=ON -DBUILD_DOC=OFF
   cmake --build build --parallel 2

Before submitting a pull request, run ``clang-format`` on any modified
source files so they conform to the project's ``.clang-format``
configuration:

.. code-block:: bash

   clang-format -i src/*.cpp src/*.h

When adding new ``.cpp`` or ``.h`` files to ``src/``, also add them to
the ``PROJECT_SOURCES`` list in the top-level ``CMakeLists.txt`` so
they are picked up by the build (Qt's ``AUTOMOC`` handles ``moc``
generation automatically once the file is listed).

All documentation should be written in American English using plain
ASCII characters (no typographic quotes, em-dashes written as ``--``,
etc.).

Contributing
============

To contribute to LAMMPS-GUI:

1. Fork the repository on GitHub
2. Create a feature branch
3. Make your changes with proper documentation
4. Ensure code compiles with Qt 6.2 or later
5. Test your changes thoroughly
6. Submit a pull request

All contributions must:

- Follow the existing code style and pass ``clang-format``
- Include Doxygen documentation for new public APIs
- Register new source files in ``PROJECT_SOURCES`` in ``CMakeLists.txt``
- Not break existing functionality
- Have GPG-signed commits
