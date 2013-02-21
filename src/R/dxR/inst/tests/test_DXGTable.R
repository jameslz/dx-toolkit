context("DXGTable")

test_that("constructor works", {
  dxgtable <- DXGTable("gtable-12345678901234567890abcd",
                       describe=FALSE)
  expect_that(dxgtable@id, equals("gtable-12345678901234567890abcd"))

  dxgtable <- DXGTable("gtable-12345678901234567890abcd",
                       project="project-12345678901234567890dcba",
                       describe=FALSE)
  expect_that(dxgtable@id, equals("gtable-12345678901234567890abcd"))
  expect_that(dxgtable@project, equals("project-12345678901234567890dcba"))

  dxgtable <- DXGTable("gtable-12345678901234567890abcd",
                       project="container-12345678901234567890dcba",
                       describe=FALSE)
  expect_that(dxgtable@id, equals("gtable-12345678901234567890abcd"))
  expect_that(dxgtable@project, equals("container-12345678901234567890dcba"))
})

test_that("validator works", {
  expect_that(DXGTable("gtable-12345678901234567890abc!", describe=FALSE),
              throws_error("invalid class"))
  expect_that(DXGTable("foo-12345678901234567890abcd", describe=FALSE),
              throws_error("invalid class"))
  expect_that(DXGTable("gtable-12345678901234567890abcd",
                       project="gtable-12345678901234567890abcd",
                       describe=FALSE),
              throws_error("invalid class"))
})

test_that("id method works", {
  dxgtable <- DXGTable("gtable-12345678901234567890abcd", describe=FALSE)
  expect_that(id(dxgtable), equals("gtable-12345678901234567890abcd"))
})

test_that("desc<- method works", {
  dxgtable <- DXGTable("gtable-12345678901234567890abcd", describe=FALSE)
  desc(dxgtable) <- list(foo="foo", bar="bar")
  expect_that(dxgtable@desc$foo, equals("foo"))
  expect_that(dxgtable@desc$bar, equals("bar"))
})
