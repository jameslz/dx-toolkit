# Testing R Bindings

Tests that do not require an Internet connection and/or access to a
DNAnexus API server should be put in the appropriate R packages so
that they can be run automatically via `R CMD check`.  Otherwise, all
such integration tests should be placed in this directory.

## Integration Tests

For now, integration tests are written with the assumption that there
is a valid dx environment to be loaded, including a valid security
context and current project in which CONTRIBUTE+ permissions are
available.  (These requirements may be relaxed over time as tests
mature.)

To run tests in this directory, run the following commands in R after
installing dxR.

    > library(testthat)
    > test_dir("/path/to/this/dir")
