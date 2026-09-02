--[[
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
]]

--[[
 crypto - crypto_encrypt_data/crypto_decrypt_data
 (docs/lua-api/crypto.html), the content-encryption primitives behind
 tools/pack.lua (see the root README.md's own "Content encryption"
 section for the full design). Both primitives are keyed entirely by
 SCARAB_CONTENT_KEY, a compile-time secret - never a Lua-supplied
 value - so this sample's own behavior depends on how the running
 `scarab` binary was actually built:

   - Default build (no key configured, cmake's own default): both
     primitives return nil and log a warning to stderr - "content
     encryption disabled" is a normal, expected state, not an error,
     so this sample shows that plainly rather than crashing on the nil.

   - Built with -DSCARAB_CONTENT_KEY=<64 hex chars>: a genuine
     encrypt -> decrypt roundtrip, plus a deliberate tamper test
     (flip one ciphertext byte) showing crypto_decrypt_data's
     authenticated-encryption failure mode - nil, not silently wrong
     data.

 Run this sample once against a default build and once against a
 build configured with a real key (see tools/README.md's own "1.
 Build scarab with your own key" step) to see both code paths.
]]

app_set_name( "Scarab - crypto sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local plaintext = "the quick brown fox jumps over the lazy dog"

local encrypted = crypto_encrypt_data( plaintext )

local status, decrypted, tamperStatus

if encrypted == nil then
    status = "crypto_encrypt_data returned nil - this build has no SCARAB_CONTENT_KEY configured (the default)."
else
    decrypted = crypto_decrypt_data( encrypted )

    if decrypted == plaintext then
        status = "roundtrip OK - encrypted " .. #plaintext .. " -> " .. #encrypted
            .. " bytes (24-byte nonce + 16-byte MAC overhead), decrypted back to the original."
    else
        -- Should never happen with a correctly-configured key and an
        -- untampered blob - flagged loudly rather than silently, since
        -- reaching this branch would itself be a real engine bug.
        status = "UNEXPECTED - decrypted data did not match the original plaintext."
    end

    -- Tamper test: flip one byte well past the nonce, inside the actual
    -- ciphertext, and confirm decryption is REFUSED rather than
    -- returning silently-wrong data - crypto_secretbox's own
    -- authenticated-encryption guarantee.
    local tampered = string.sub( encrypted, 1, 30 )
        .. string.char( ( string.byte( encrypted, 31 ) + 1 ) % 256 )
        .. string.sub( encrypted, 32 )
    local tamperResult = crypto_decrypt_data( tampered )

    if tamperResult == nil then
        tamperStatus = "tamper test OK - one flipped ciphertext byte correctly refused (nil), not silently accepted."
    else
        tamperStatus = "UNEXPECTED - a tampered blob was accepted."
    end
end

function on_update( dt )

    draw_text( "crypto sample", 20, 20, 24, 255, 255, 255, 255 )
    draw_text( status, 20, 54, 18, 200, 200, 200, 255 )

    if tamperStatus ~= nil then
        draw_text( tamperStatus, 20, 82, 18, 200, 200, 200, 255 )
    end

    draw_text( "Rebuild with -DSCARAB_CONTENT_KEY=<64 hex chars> to see the encrypted path (see tools/README.md).",
        20, screen_get_height() - 34, 16, 140, 140, 140, 255 )
end
