# Clubs CSV Format for FIFA Import Tool

## Required Columns

| Column | Type | Description | Max Length |
|--------|------|-------------|------------|
| club_id | integer | Unique identifier for linking players | - |
| club_name | string | Club name | 16 chars |
| manager_name | string | Manager name | 16 chars |
| stadium_name | string | Stadium name | 24 chars |
| league | integer | League tier (0=Premier, 1=Div1, 2=Div2, 3=Div3, 4=Conference) | 0-4 |
| max_players | integer | Maximum players to import for this club (optional, uses global default if omitted) | 1-24 |

## Kit Colors (Home Kit)

| Column | Type | Description | Range |
|--------|------|-------------|-------|
| home_shirt_design | integer | Shirt pattern/design | 0-255 |
| home_shirt_primary_r | integer | Primary shirt color - Red | 0-15 |
| home_shirt_primary_g | integer | Primary shirt color - Green | 0-15 |
| home_shirt_primary_b | integer | Primary shirt color - Blue | 0-15 |
| home_shirt_secondary_r | integer | Secondary shirt color - Red | 0-15 |
| home_shirt_secondary_g | integer | Secondary shirt color - Green | 0-15 |
| home_shirt_secondary_b | integer | Secondary shirt color - Blue | 0-15 |
| home_shorts_r | integer | Shorts color - Red | 0-15 |
| home_shorts_g | integer | Shorts color - Green | 0-15 |
| home_shorts_b | integer | Shorts color - Blue | 0-15 |
| home_socks_r | integer | Socks color - Red | 0-15 |
| home_socks_g | integer | Socks color - Green | 0-15 |
| home_socks_b | integer | Socks color - Blue | 0-15 |

## Optional: Away Kits (away1_, away2_ prefixes)

Same format as home kit but with `away1_` and `away2_` prefixes. If not provided, home kit will be copied to all three slots.

## Example

```csv
club_id,club_name,manager_name,stadium_name,league,max_players,home_shirt_design,home_shirt_primary_r,home_shirt_primary_g,home_shirt_primary_b,home_shirt_secondary_r,home_shirt_secondary_g,home_shirt_secondary_b,home_shorts_r,home_shorts_g,home_shorts_b,home_socks_r,home_socks_g,home_socks_b
1,Arsenal,Mikel Arteta,Emirates Stadium,0,20,1,15,0,0,15,15,15,15,15,15,15,0,0
2,Liverpool,Jurgen Klopp,Anfield,0,18,0,15,0,0,15,15,0,15,0,0,15,0,0
3,Smalltown FC,John Smith,Smalltown Ground,4,12,0,0,0,15,15,15,15,0,0,15,0,0,15
```

## Notes

- Maximum 244 clubs
- Colors use 4-bit values (0-15), where 0=black, 15=white
- Common colors: Red (15,0,0), Blue (0,0,15), White (15,15,15), Black (0,0,0)
- Shirt design: 0=solid, 1=stripes, 2=hoops, etc. (see PM3 docs for full list)
- `max_players` is optional per club - if omitted or 0, uses the global `--max-players` flag (default 16)
- This allows flexible squad sizes: big clubs can have 20-24 players, smaller clubs 10-12 players
