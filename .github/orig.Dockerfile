# A private image carrying nothing but the retail DOL, for CI.
#
# Everything else the build needs -- binutils, the CodeWarrior compilers, dtk,
# objdiff-cli and wibo -- configure.py downloads from public URLs on its own, so
# the only thing CI cannot obtain for itself is the 2.8 MB of retail code in
# orig/GQPE78/sys/main.dol.
#
# The point of owning this image is that it needs no personal access token.
# ghcr.io/bfbbdecomp/bfbb-build is private to another organisation, and a fork's
# GITHUB_TOKEN has scope over its own repository only, which is why pulling it
# here fails with a bare "denied". A package in YOUR namespace can be granted to
# YOUR repository, and then the automatic GITHUB_TOKEN is enough.
#
# Build and publish it once, from a checkout that has orig/ populated:
#
#   docker build -f .github/orig.Dockerfile -t ghcr.io/<you>/bfbb-orig:main .
#   echo $CR_PAT | docker login ghcr.io -u <you> --password-stdin
#   docker push ghcr.io/<you>/bfbb-orig:main
#
# (That one push needs a token with write:packages, from your machine. CI does
# not.)
#
# No container runtime? The image is one layer, one config blob and a manifest,
# so it can be pushed straight to the registry API -- see the scratch script
# recorded in docs/DUPLOTRON.md. That is how the current image was published.
#
# Then, once, on github.com:
#   Your profile -> Packages -> bfbb-orig -> Package settings
#     - Danger Zone -> Change visibility -> Private
#     - Manage Actions access -> Add repository -> <you>/bfbb, role: Read
#   Repo -> Settings -> Secrets and variables -> Actions -> Variables
#     - New variable: HAVE_ORIG_IMAGE = true
#
# Refresh it only if the retail image itself ever changes, which it will not.

FROM scratch
COPY orig/ /orig/
