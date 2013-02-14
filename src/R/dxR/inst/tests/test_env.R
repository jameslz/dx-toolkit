context("environment")

test_that("loadFromEnvironment respects new env vars", {
  Sys.setenv(DX_APISERVER_HOST="foo")
  loadFromEnvironment()
  expect_that(dxR:::dxEnv$DX_APISERVER_HOST, equals("foo"))
})
