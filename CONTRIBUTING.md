# Contributing to poski

Welcome! We are excited that you want to contribute to `poski`, the portable C++ OSAL abstraction library for Matter. 

Before contributing, please read this page. By submitting a pull request, you represent that you have the right to license your contribution to the Connectivity Standards Alliance and the community, and you agree that your contributions are licensed under the [Apache 2.0 License](./LICENSE).

---

## Requirements for Contributing

To contribute code or documentation to the repository, you must:

1.  Agree to the [Code of Conduct](./CODE_OF_CONDUCT.md).
2.  Agree to the [License](./LICENSE) (Apache 2.0).
3.  Sign the [Matter Working Group CLA](https://gist.github.com/clapre/65aa9fc63981da765039e0bb7e8701be).

---

## Contribution Workflow

We use the standard "Fork-and-Pull" model for managing contributions:

### 1. Fork and Clone
Fork the repository on GitHub and clone your fork locally:
```bash
git clone git@github.com:<your-username>/poski.git
cd poski
git remote add upstream git@github.com:project-chip/poski.git
```

### 2. Create a Branch
Create a descriptive feature branch off the `main` branch:
```bash
git checkout -b feature/my-new-abstraction
```

### 3. Author Code & Verify
*   **Code Quality:** Write clean C/C++ conforming to standard guidelines.
*   **Copyright Headers:** Every brand-new file must include the standard Apache 2.0 copyright header attributing `Project CHIP Authors`. Do not modify historical copyrights in existing files.
*   **Validation:** Verify that all test suites build cleanly and pass under both Make and Bazel pipelines:
    ```bash
    # Make Pipeline
    make -C tests clean && make -C tests test && make -C tests test_gtest

    # Bazel Pipeline
    bazel clean --expunge && bazel test //:test //:gtest
    ```

### 4. Submit a Pull Request
*   Push your branch to your fork:
    ```bash
    git push origin feature/my-new-abstraction
    ```
*   Submit a Pull Request against `project-chip/poski:main`.
*   Ensure all CI checks pass successfully. Once a project maintainer reviews and approves the change, it will be merged into `main`.

---

## Feedback and Bugs
If you encounter a bug, have a question, or would like to propose a feature request, please [submit a GitHub Issue](https://github.com/project-chip/poski/issues/new).
