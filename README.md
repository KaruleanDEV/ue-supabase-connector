# SupabaseConnector (Unreal Engine)

A lightweight Unreal Engine C++ plugin for Supabase authentication (email/password login and signup) with Blueprint-friendly async callbacks.

## Features

- `LoginUserAsync(Email, Password, OnSuccess, OnFailure)`
- `RegisterUserAsync(Email, Password, OnSuccess, OnFailure)`
- Uses Unreal HTTP module + JSON serialization
- Configurable credentials via environment variables or `DefaultGame.ini`
- Error chaining with parsed Supabase response message

## Required headers

- `SupabaseConnector.h` (Blueprint callable entry points)
- `SupabaseConnector.cpp` (implementation)
- `SupabaseStructs.h` (response structs)

## Production Configuration

### 1) Environment Variables (recommended)

Set:
- `SUPABASE_URL` (e.g. `https://yourproject.supabase.co/`)
- `SUPABASE_APIKEY` (Supabase anon/public API key)

### 2) DefaultGame.ini fallback

```ini
[Supabase]
SupabaseUrl=https://yourproject.supabase.co/
SupabaseApiKey=your_api_key
```

> Ensure `SupabaseUrl` ends with `/` (helper function appends if missing).

## Usage (Blueprint)

1. Place `USupabaseAuthLibrary` functions in Blueprint graph.
2. Bind `FOnLoginResponse` to success handler.
3. Bind `FOnAuthFailure` to error handler.
4. Call `LoginUserAsync` or `RegisterUserAsync`.

### Example success data (JSON -> struct)
- `FSupabaseLoginResponse.access_token`
- `FSupabaseLoginResponse.refresh_token`
- `FSupabaseLoginResponse.expires_in`
- `FSupabaseLoginResponse.token_type`
- `FSupabaseLoginResponse.user.id`
- `FSupabaseLoginResponse.user.email`

## Error handling

- Connection error / timeout -> `OnFailure("Supabase settings are not configured.")` or timeout message
- Non-2xx responses -> parsed Supabase error (`msg`, `error_description`, `error`), otherwise raw response
- JSON deserialization failures -> `OnFailure("Failed to parse ... JSON")`

## Development / build notes

- Ensure you import `Http` module in `.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Http", "Json", "JsonUtilities" });
```

- Add `SupabaseConnector.cpp` and header files to your plugin/module source.

## Security notes
- Use `supabase-auth` policies and row-level security to protect data access.
