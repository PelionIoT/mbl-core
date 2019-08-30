import pytest
import re


def pytest_addoption(parser):
    """Add option parsing to pytest."""
    parser.addoption("--local-conf", action="store", default=None)


@pytest.fixture
def local_conf_path(request):
    """Fixture for --local-conf."""
    return request.config.getoption("--local-conf")


def get_local_conf_assignments_dict(local_conf_path):
    """
    Get a list of the variable (name, value) pairs for each simple assignment
    in the given local.conf.

    Note that this function only deals with values that are double quoted and
    doesn't deal with override syntax, variable appends, variable prepends or
    "weak" assignments.

    I.e. the variable values returned by this function are NOT guaranteed to be
    final values used by BitBake.

    Returns a dict mapping variable names to the values assigned to them.
    """
    assignment_re = re.compile(
        r"""^\s*
            (?P<var_name> [A-Za-z0-9_]+ )
            \s* = \s*
            "(?P<var_value> [^"]* )"
            \s*
        $""",
        re.X,
    )
    assignments = {}
    with open(local_conf_path, "r") as config:
        for line in config:
            m = assignment_re.match(line)
            if m:
                # Don't check if the var name is already in our dict - just let
                # later assignments take precedence.
                assignments[m["var_name"]] = m["var_value"]
    return assignments


@pytest.fixture
def local_conf_assignments_dict(local_conf_path):
    """
    Fixture for the dictionary mapping variable names to their values for
    assignments in local.conf.

    The caveats in the docstring for get_local_conf_assignments_dict apply here
    too.
    """
    if local_conf_path is None:
        return {}
    return get_local_conf_assignments_dict(local_conf_path)


def pytest_generate_tests(metafunc):
    """
    Hook to allow us to modify tests as they are collected.
    """
    # Allow tests to be parameterized on local.conf assignments.
    # We can't do this using a normal fixture because fixtures aren't available
    # at test collection time when the list of parameters is required.
    assignments = []
    local_conf_path = metafunc.config.getoption("--local-conf")
    if local_conf_path is not None:
        assignments = get_local_conf_assignments_dict(local_conf_path).items()

    if "local_conf_assignment" in metafunc.fixturenames:
        metafunc.parametrize(
            "local_conf_assignment",
            assignments,
            ids=[a[0] for a in assignments],
        )
