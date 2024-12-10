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
async def create_all_items(service_client, auth_headers):
    data = {
        "name": "test_room"
    }
    room_response = await service_client.post(
        '/v1/rooms',
        headers=auth_headers,
        json=data
    )
    assert room_response.status == 200
    data = {
        "name": "test_product",
        "price": 12000,
        "room_id": 1
    }
    product_response = await service_client.post(
        '/v1/products',
        headers=auth_headers,
        json=data
    )
    assert product_response.status == 200
    data = {
        "product_id": 1,
        "user_id": 1
    }
    user_product_response = await service_client.post(
        '/v1/user-products',
        headers=auth_headers,
        json=data
    )
    assert user_product_response.status == 200
    return auth_headers


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


@pytest.fixture
async def setup_product(service_client, setup_room):
    data = {
        "name": "test_product",
        "price": 12000,
        "room_id": 1
    }
    response = await service_client.post(
        '/v1/products',
        headers=setup_room,
        json=data
    )
    assert response.status == 200
    return setup_room


@pytest.fixture
async def create_user_product_headers(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 1
    }
    response = await service_client.post(
        '/v1/user-products',
        headers=setup_product,
        json=data
    )
    assert response.status == 200
    return setup_product

@pytest.mark.asyncio
async def test_create_product(service_client, setup_room):
    data = {
        "name": "test",
        "price": 12000,
        "room_id": 1
    }
    response = await  service_client.post(
        "v1/products",
        headers=setup_room,
        json=data
    )
    assert response.status == 200
    response_data = response.json()

    assert response_data["id"] is not None
    assert response_data["name"] == data["name"]
    assert response_data["price"] == data["price"]
    assert response_data["room_id"] == data["room_id"]

@pytest.mark.asyncio
async def test_get_product(service_client, setup_product):
    product_id = 1
    response = await service_client.get(f"/v1/products/{product_id}", headers=setup_product)
    assert response.status == 200
    response_data = response.json()
    assert response_data["id"] == 1
    assert response_data["name"] == "test_product"
    assert response_data["price"] == 12000
    assert response_data["room_id"] == 1

# @pytest.mark.asyncio
# async def test_get_all_products(service_client, auth_headers, setup_product):
#     response = await service_client.get('/v1/products', headers=auth_headers)
#     assert response.status == 200
#     response_data = response.json()
#     assert any(product["id"] == setup_product['id'] for product in response_data)

@pytest.mark.asyncio
async def test_delete_product(service_client, setup_product):
    product_id = 1
    response = await service_client.delete(f"/v1/products/{product_id}", headers=setup_product)
    assert response.status == 200
    response_data = response.json()
    assert response_data["id"] == product_id
    assert response_data["status"] == "deleted"

@pytest.mark.asyncio
async def test_get_nonexistent_product(service_client, setup_product):
    response = await service_client.get('/v1/products/9999', headers=setup_product)
    assert response.status == 404  # Not Found

@pytest.mark.asyncio
async def test_delete_nonexistent_product(service_client, setup_product):
    response = await service_client.delete('/v1/products/9999', headers=setup_product)
    assert response.status == 404

@pytest.mark.asyncio
async def test_add_product_invalid_room(service_client, setup_room):
    data = {
        "name": "invalid_room_product",
        "price": 20000,
        "room_id": 9999
    }
    response = await service_client.post('/v1/products', headers=setup_room, json=data)
    assert response.status == 404
    response_data = response.json()
    assert response_data["error"] == "Room ID is Invalid!"
