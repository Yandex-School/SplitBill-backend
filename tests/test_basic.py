import pytest
import aiohttp
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

@pytest.fixture
async def register_headers(service_client):
    data = {
        "username": "test_user",
        "password": "test_password"
    }
    response = await service_client.post(
        '/register',
        headers={'Content-Type': "application/json"},
        json=data
    )
    assert response.status == 200


@pytest.fixture
async def auth_headers(service_client):
    data = {
        "username": "test_user",
        "password": "test_password"
    }
    response = await service_client.post(
        '/register',
        headers={'Content-Type': "application/json"},
        json=data
    )
    assert response.status == 200

    response = await service_client.post(
        '/login',
        headers={'Content-Type': "application/json"},
        json=data
    )
    assert response.status == 200

    response_data = response.json()
    token = response_data['id']

    return {"X-Ya-User-Ticket": f"{token}"}



@pytest.mark.asyncio
async def test_register_user(service_client):
    data = {
        "username": "new_user",
        "password": "new_password",
        "full_name": "test test",
        "photo_url": "some.url"
    }
    response = await service_client.post('/register', headers={'Content-Type': 'application/json'}, json=data)
    assert response.status == 200
    response_data = response.json()
    response_data["id"] is not None


@pytest.mark.asyncio
async def test_login_user(service_client,register_headers):
    data = {
        "username": "test_user",
        "password": "test_password"
    }
    response = await service_client.post('/login', headers={'Content-Type': 'application/json'}, json=data)
    assert response.status == 200
    assert "id" in response.json()

# @pytest.mark.asyncio
# async def test_register_user_existing_username(service_client,register_headers):
#     data = {
#         "username": "test_user",
#         "password": "password123"
#     }
#     response = await service_client.post('/register', headers={'Content-Type': 'application/json'}, json=data)
#     assert response.status == 400
#
# @pytest.mark.asyncio
# async def test_login_user_invalid_credentials(service_client,register_headers):
#     data = {
#         "username": "invalid_user",
#         "password": "wrong_password"
#     }
#     response = await service_client.post('/login', headers={'Content-Type': 'application/json'}, json=data)
#     assert response.status == 401  # Unauthorized
#
