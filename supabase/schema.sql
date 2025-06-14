-- Supabase Schema for Automatic IoT Plant Irrigation System

DROP TABLE IF EXISTS irrigation_data CASCADE;
DROP TABLE IF EXISTS irrigation_commands CASCADE;

-- Create a table to store sensor data from ESP32 devices
CREATE TABLE IF NOT EXISTS irrigation_data (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, -- Changed
    timestamp TIMESTAMPTZ DEFAULT NOW(), -- Changed from created_at
    device_id VARCHAR(255) NOT NULL, -- Unique identifier for each ESP32 sensor node (e.g., MAC address)
    weight NUMERIC, -- Plant weight, can be NULL if sensor not present or fails
    moisture NUMERIC, -- Soil moisture percentage, can be NULL
    temperature NUMERIC, -- Optional: Ambient temperature
    humidity NUMERIC, -- Optional: Ambient humidity
    -- Add any other sensor fields you might need
    metadata JSONB -- For any other unstructured data
);

-- Create a table to store irrigation commands for ESP32 actuator nodes
CREATE TABLE IF NOT EXISTS irrigation_commands (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, -- Changed
    timestamp TIMESTAMPTZ DEFAULT NOW(), -- Changed from created_at
    device_id VARCHAR(255) NOT NULL, -- Target device ID for the command (actuator's unique ID)
    start BOOLEAN NOT NULL, -- TRUE to start irrigation, FALSE to stop
    duration_seconds INTEGER, -- Optional: For how long to irrigate
    command_source VARCHAR(50) DEFAULT 'dashboard', -- e.g., 'dashboard', 'automatic_schedule', 'manual'
    acknowledged BOOLEAN DEFAULT FALSE -- Actuator can set this to TRUE once command is processed
);

-- Create RLS (Row Level Security) policies

-- For irrigation_data table:
-- Allow anonymous authenticated users (like ESP32 devices with an anon key)
-- to insert their own sensor readings.
ALTER TABLE irrigation_data ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Allow anon insert for own device data" ON irrigation_data;
CREATE POLICY "Allow anon insert for own device data"
    ON irrigation_data
    FOR INSERT
    TO anon, authenticated
    WITH CHECK (true); -- Or more restrictively: WITH CHECK (device_id = current_setting('request.jwt.claims', true)::jsonb->>'device_id') if using custom JWTs

DROP POLICY IF EXISTS "Allow all read access for dashboard/analysis" ON irrigation_data;
CREATE POLICY "Allow all read access for dashboard/analysis"
    ON irrigation_data
    FOR SELECT
    TO anon, authenticated -- Adjust as needed, maybe only to a specific 'dashboard_user' role
    USING (true);

-- For irrigation_commands table:
-- Allow actuators to read commands intended for them.
-- Allow a dashboard/admin role to insert/update commands.
ALTER TABLE irrigation_commands ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Allow actuator to read its commands" ON irrigation_commands;
CREATE POLICY "Allow actuator to read its commands"
    ON irrigation_commands
    FOR SELECT
    TO anon, authenticated -- Assuming actuator uses anon/authenticated key
    USING (true); -- Actuator will filter by device_id in its query. Or use: (device_id = current_setting('request.jwt.claims', true)::jsonb->>'device_id') if JWT contains device_id.

DROP POLICY IF EXISTS "Allow actuator to acknowledge its commands" ON irrigation_commands; -- Corrected name
CREATE POLICY "Allow actuator to acknowledge its commands" -- Renamed policy
    ON irrigation_commands
    FOR UPDATE
    TO anon, authenticated
    USING (true) -- Or (device_id = current_setting('request.jwt.claims', true)::jsonb->>'device_id')
    WITH CHECK (true); -- Allows updating any field. For more security, restrict to only 'acknowledged' column and specific device_id.

DROP POLICY IF EXISTS "Allow dashboard/admin to manage commands" ON irrigation_commands;
CREATE POLICY "Allow dashboard/admin to manage commands"
    ON irrigation_commands
    FOR ALL
    TO service_role -- Or a specific 'dashboard_user' role
    USING (true)
    WITH CHECK (true);

-- Example of how to add a specific role if you want more granular control than just 'anon' or 'authenticated'
-- CREATE ROLE dashboard_user;
-- GRANT USAGE ON SCHEMA public TO dashboard_user;
-- GRANT SELECT, INSERT, UPDATE, DELETE ON irrigation_data TO dashboard_user;
-- GRANT SELECT, INSERT, UPDATE, DELETE ON irrigation_commands TO dashboard_user;
-- GRANT USAGE, SELECT ON SEQUENCE irrigation_data_id_seq TO dashboard_user;
-- GRANT USAGE, SELECT ON SEQUENCE irrigation_commands_id_seq TO dashboard_user;


-- Add comments to tables and columns for better understanding
COMMENT ON TABLE irrigation_data IS 'Stores sensor readings from ESP32 devices, like soil moisture and plant weight.';
COMMENT ON COLUMN irrigation_data.device_id IS 'Unique ID of the sensor device, typically its MAC address.';
COMMENT ON COLUMN irrigation_data.timestamp IS 'Timestamp of when the data was recorded.'; -- Update to timestamp

COMMENT ON TABLE irrigation_commands IS 'Stores commands to control irrigation actuators.';
COMMENT ON COLUMN irrigation_commands.device_id IS 'Unique ID of the target actuator device.';
COMMENT ON COLUMN irrigation_commands.start IS 'True to start watering, False to stop.';
COMMENT ON COLUMN irrigation_commands.timestamp IS 'Timestamp of when the command was created.'; -- Update to timestamp

-- Consider adding indexes for frequently queried columns, e.g., device_id and created_at
CREATE INDEX IF NOT EXISTS idx_irrigation_data_device_id ON irrigation_data(device_id);
CREATE INDEX IF NOT EXISTS idx_irrigation_data_timestamp ON irrigation_data(timestamp DESC); -- Update to timestamp

CREATE INDEX IF NOT EXISTS idx_irrigation_commands_device_id ON irrigation_commands(device_id);
CREATE INDEX IF NOT EXISTS idx_irrigation_commands_timestamp ON irrigation_commands(timestamp DESC); -- Update to timestamp


-- Note on API Keys for ESP32:
-- It is recommended to use the 'anon' key for ESP32 devices for public-facing operations.
-- RLS policies should then be configured to grant appropriate permissions to the 'anon' role.
-- The 'service_role' key should be kept secret and used for administrative tasks or backend services
-- that require bypassing RLS (e.g., the dashboard backend if it needs elevated privileges).

-- After running this schema in your Supabase SQL editor:
-- 1. Go to API settings and get your Project URL and anon key.
-- 2. Update your ESP32 firmware with these credentials.
-- 3. For the actuator, ensure it queries for commands matching its specific device_id if you use a generic anon key and `USING (true)` for reads.
--    Alternatively, if your ESP32s can authenticate with a JWT that includes their device_id, the RLS policy `USING (device_id = current_setting('request.jwt.claims', true)::jsonb->>'device_id')` can be very effective.
