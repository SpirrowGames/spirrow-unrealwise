"""
pytest configuration and fixtures for SpirrowUnrealWise tests
"""

import os
import pytest
import logging
from test_framework import UnrealMCPClient, TestSuite

# ロギング設定
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)


@pytest.fixture(scope="session")
def mcp_client():
    """セッション全体で共有するMCPクライアント"""
    client = UnrealMCPClient(
        host=os.getenv("UNREAL_HOST", "127.0.0.1"),
        port=int(os.getenv("UNREAL_PORT", "55557")),
        timeout=15.0,
    )
    yield client
    client.disconnect()


@pytest.fixture
def test_suite(mcp_client):
    """各テスト用のテストスイート"""
    suite = TestSuite(mcp_client)
    yield suite
    suite.cleanup()


@pytest.fixture
def unique_name():
    """ユニークな名前を生成するファクトリ"""
    import uuid
    def _generate(prefix: str = "Test") -> str:
        return f"{prefix}_{uuid.uuid4().hex[:8]}"
    return _generate


# pytest markers
def pytest_configure(config):
    """カスタムマーカーを登録"""
    config.addinivalue_line("markers", "actor: Actor操作テスト")
    config.addinivalue_line("markers", "blueprint: Blueprint操作テスト")
    config.addinivalue_line("markers", "umg: UMG Widget操作テスト")
    config.addinivalue_line("markers", "node: Blueprintノード操作テスト")
    config.addinivalue_line("markers", "gas: GAS操作テスト")
    config.addinivalue_line("markers", "slow: 遅いテスト")
    config.addinivalue_line("markers", "integration: 統合テスト")
