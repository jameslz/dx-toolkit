context("environment")

test_that("loadFromEnvironment respects new env vars", {
  Sys.setenv(DX_APISERVER_HOST="foo")
  loadFromEnvironment()
  expect_that(dxR:::dxEnv$DX_APISERVER_HOST, equals("foo"))
})

test_that("DEFAULT_CONTAINER is set properly", {
  Sys.setenv(DX_JOB_ID="job-xxxx")
  Sys.setenv(DX_PROJECT_CONTEXT_ID="project-xxxx")
  Sys.setenv(DX_WORKSPACE_ID="container-xxxx")

  loadFromEnvironment()
  expect_that(dxR:::dxEnv$DEFAULT_CONTAINER, equals("container-xxxx"))

  Sys.unsetenv("DX_JOB_ID")
  loadFromEnvironment()
  expect_that(dxR:::dxEnv$DEFAULT_CONTAINER, equals("project-xxxx"))
})
