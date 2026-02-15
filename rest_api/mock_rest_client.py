import requests

class RestApiClient:
    def __init__(self, ip_port):
        self.base_url = f"http://{ip_port}"

    def read_all(self, table):
        url = f"{self.base_url}/read/{table}"
        response = requests.get(url)
        return self._handle_response(response)

    def read_by_id(self, table, entity_id):
        url = f"{self.base_url}/read/{table}/{entity_id}"
        response = requests.get(url)
        return self._handle_response(response)

    def create_entity(self, table, data):
        url = f"{self.base_url}/insert/{table}"
        response = requests.post(url, json=[data])
        return self._handle_response(response)

    def update_entity(self, table, entity_id, data):
        url = f"{self.base_url}/update/{table}/{entity_id}"
        response = requests.put(url, json=data)
        return self._handle_response(response)

    def delete_entity(self, table, entity_id):
        url = f"{self.base_url}/delete/{table}/{entity_id}"
        response = requests.delete(url)
        return self._handle_response(response)

    def join_entity(self, left_table, right_table):
        url = f"{self.base_url}/join/{left_table}/{right_table}"
        response = requests.get(url)
        return self._handle_response(response)

    def order_entity(self, table, column, order="asc"):
        url = f"{self.base_url}/order/{table}/{column}/{order}"
        response = requests.get(url)
        return self._handle_response(response)

    @staticmethod
    def _handle_response(response):
        """Utility to safely parse JSON or print error."""
        try:
            print(response.status_code)
            return response.text
        except requests.HTTPError as e:
            print(f"HTTP error: {e}, Response: {response.text}")
        except ValueError:
            print("Invalid JSON received.")
        return None


if __name__ == "__main__":
    client = RestApiClient("172.20.0.3:3004")

    # Example usage:
    print(client.read_all("users"))
    print(client.read_by_id("users", 1))
    print(client.create_entity("users", {"name": "Alice"}))
    print(client.update_entity("users", 1, {"name": "UpdatedAlice"}))
    print(client.delete_entity("users", 13))
    print(client.join_entity("users", "orders"))
    print(client.order_entity("users", "name", "asc"))

