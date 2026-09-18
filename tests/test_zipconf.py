import os
import pytest

from tll.channel import Context
from tll.config import Config
from tll.scheme import Scheme

@pytest.fixture(scope='session')
def zipload():
    ctx = Context()
    ctx.load(os.path.join(os.environ.get("BUILD_DIR", "build"), "tll-zipconf"))
    return ctx

@pytest.mark.parametrize("file", ["config.yaml", "yamls", "gz"])
def test(zipload, file):
    cfg = Config.load(f"zip://tests/config.zip::dir/{file}")
    assert cfg.as_dict() == {'a': 'b', 'c': 'd'}

def test_scheme(zipload):
    s = Scheme('zip://tests/config.zip::dir/scheme')
    assert [m.name for m in s.messages] == ['Data']
