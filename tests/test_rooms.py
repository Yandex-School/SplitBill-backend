import pytest
import aiohttp
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


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

@pytest.fixture
async def setup_room(service_client, auth_headers):
    data = {
        "name": "test_room"
    }
    response = await service_client.post(
        '/v1/rooms',
        headers=auth_headers,
        json=data
    )
    assert response.status == 200
    return auth_headers



@pytest.mark.asyncio
async def test_create_room(service_client, auth_headers):
    data = {"name": "new_room"}
    response = await service_client.post('/v1/rooms', headers=auth_headers, json=data)
    assert response.status == 200
    assert response.json()["name"] == data["name"]

@pytest.mark.asyncio
async def test_join_room(service_client, setup_room):
    room_id = 1
    response = await service_client.post(f"/v1/rooms/join/{room_id}", headers=setup_room)
    assert response.status == 200


@pytest.mark.asyncio
async def test_get_all_rooms(service_client, setup_room):
    response = await service_client.get('/v1/rooms/', headers=setup_room)
    assert response.status == 200
#
# @pytest.mark.asyncio
# async def test_get_room_users(service_client, auth_headers, setup_room):
#     response = await service_client.get(f"/v1/rooms/{setup_room['id']}/users", headers=auth_headers)
#     assert response.status == 200
#     assert "users" in response.json()
#
@pytest.mark.asyncio
async def test_update_room(service_client, setup_room):
    room_id = 1
    data = {"name": "updated_room"}
    response = await service_client.put(f"/v1/rooms/{room_id}", headers=setup_room, json=data)
    assert response.status == 200
    assert response.json()["status"] == "success"


@pytest.mark.asyncio
async def test_join_nonexistent_room(service_client, setup_room):
    response = await service_client.post('/v1/rooms/join/9999', headers=setup_room)
    assert response.status == 404

@pytest.mark.asyncio
async def test_update_room_nonexistent(service_client, setup_room):
    data = {"name": "nonexistent_room"}
    response = await service_client.put('/v1/rooms/9999', headers=setup_room, json=data)
    assert response.status == 404

# @pytest.mark.asyncio
# async def test_get_room_users_unauthorized(service_client, setup_room):
#     response = await service_client.get(f"/v1/rooms/{setup_room['id']}/users", headers={})
#     assert response.status == 403  # Unauthorized
