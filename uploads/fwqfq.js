import { BlockVolume, ItemStack, system, world, Direction, EquipmentSlot, EnchantmentType, ItemComponentTypes } from "@minecraft/server";
import { ActionFormData } from "@minecraft/server-ui";

const ore = {
    "minecraft:coal": { name: "§8Coal Ore", block_typeId: "coal_ore" },
    "minecraft:iron_ingot": { name: "§7Iron Ore", block_typeId: "iron_ore" },
    "minecraft:gold_ingot": { name: "§eGold Ore", block_typeId: "gold_ore" },
    "minecraft:redstone": { name: "§cRedstone Ore", block_typeId: "redstone_ore" },
    "minecraft:lapis_lazuli": { name: "§9Lapis Ore", block_typeId: "lapis_ore" },
    "minecraft:diamond": { name: "§lDiamond Ore", block_typeId: "diamond_ore" },
    "minecraft:emerald": { name: "§aEmerald Ore", block_typeId: "emerald_ore" },
    "minecraft:copper_ingot": { name: "§6Copper Ore", block_typeId: "copper_ore" },
};
world.beforeEvents.playerInteractWithEntity.subscribe((data) => {
    const { target, itemStack, player } = data
    if (target.typeId !== "miner:oreradar" || !itemStack) return

    const fromVector = { x: target.location.x - 10, y: target.location.y - 1, z: target.location.z - 10 }
    const toVector = { x: target.location.x + 10, y: -60, z: target.location.z + 10 }
    if (!ore[itemStack.typeId]) return
    const ore_typeId = ore[itemStack.typeId].block_typeId
    system.run(() => {
        const listBlocks = target.dimension.getBlocks(new BlockVolume(fromVector, toVector), { includeTypes: [`minecraft:${ore_typeId}`, `minecraft:deepslate_${ore_typeId}`] })
        Array.from(listBlocks.getBlockLocationIterator()).forEach((pos, i) => {
            system.runTimeout(() => {
                if (!target.isValid) return
                const block = target.dimension.getBlock(pos)
                target.dimension.spawnItem(new ItemStack(block.typeId), { x: target.location.x, y: target.location.y + 1.3, z: target.location.z })
                block.setType('minecraft:air')
                player.runCommandAsync('execute at @e[type=miner:oreradar,r=9] run particle miner:radar ~~~')
                player.runCommandAsync('execute at @e[type=miner:oreradar,r=9] run particle miner:radardot ~~~')
                player.runCommandAsync('execute at @e[type=miner:oreradar,r=9] run playsound random.fizz @p')
                player.runCommandAsync('execute at @e[type=miner:oreradar,r=9] run playsound beacon.activate @p')
                player.runCommandAsync('execute at @e[type=miner:oreradar,r=9] run tellraw @p[r=5] {\"rawtext\":[{\"text\":\"- §6Ores Have Been Found\"}]}')
            }, 70 * i)
        })
    })
})

const BACKPACK_LEVEL_ID = "bdcraft:backpack_level"

function getPlayerBackpackLevel(player) {
    return player.getDynamicProperty(BACKPACK_LEVEL_ID) ?? 1;
}

function setPlayerBackpackLevel(player, level) {
    return player.setDynamicProperty(BACKPACK_LEVEL_ID, level);
}

const levels = {
    1 : 'miner:tier_one',
    2 : 'miner:tier_two',
    3 : 'miner:tier_three',
}

class SavedItemStack {
    constructor(item) {
        // save basic stuff
        this.typeId = item.typeId;
        this.amount = item.amount;
        this.lockMode = item.lockMode;
        this.nameTag = item.nameTag;
        // save durability
        const idura = item.getComponent(ItemComponentTypes.Durability);
        if (idura)
            this.damage = idura.damage;
        // save enchants
        const ienchants = item.getComponent(ItemComponentTypes.Enchantable);
        if (ienchants)
            this.enchants = ienchants.getEnchantments();
        // save properties
        this.canDestroy = item.getCanDestroy();
        this.canPlaceOn = item.getCanPlaceOn();
        // item.getDynamicPropertyIds().forEach(propertyId => this.properties[propertyId] = JSON.stringify(item.getDynamicProperty(propertyId)));
    }
    static from(savedItemStack) {
        if (!savedItemStack)
            return null;
        const item = new ItemStack(savedItemStack.typeId, savedItemStack.amount);
        item.nameTag = savedItemStack.nameTag;
        item.setCanDestroy(savedItemStack.canDestroy);
        item.setCanPlaceOn(savedItemStack.canPlaceOn);
        item.lockMode = savedItemStack.lockMode;
        item.setLore(savedItemStack.lore);
        const idura = item.getComponent(ItemComponentTypes.Durability);
        if (idura)
            idura.damage = savedItemStack.damage;
        const ienchants = item.getComponent(ItemComponentTypes.Enchantable);
        if (ienchants)
            savedItemStack.enchants.forEach(enchant => ienchants.addEnchantment({ type: new EnchantmentType(enchant.type.id), level: enchant.level }));
        // for (const key in savedItemStack.properties)
        //     item.setDynamicProperty(key, JSON.parse(savedItemStack.properties[key]));
        return item;
    }
}

const SKIN_IDS = {
    'default' : 0,
    'red' : 1,
    'aqua' : 2,
    'gray' : 3,
    'purple' : 4,
    'green' : 5
}

function spawnBackpack(player, item, location) {
    const rideable = player.getComponent('rideable');
    const skin = SKIN_IDS[item?.typeId?.split('_')[item?.typeId?.split('_')?.length - 1]]
    const rideEntity = rideable.getRiders().find(entity => entity?.typeId == 'miner:backpack_inventory')
    if (rideEntity)
    {
        const skinId = rideEntity.getComponent('skin_id');
        if (skinId)
        {
            skinId.value = skin;
        }
        rideEntity.removeEffect('invisibility')
        rideable.ejectRiders()
        rideEntity.teleport(location)
        return rideEntity
    }
    else {
        const entity = player.dimension.spawnEntity('miner:backpack_inventory', location);
        if (!entity)
            return;
        const skinId = entity.getComponent('skin_id');
        if (skinId)
        {
            skinId.value = skin;
        }
        entity.triggerEvent(levels[getPlayerBackpackLevel(player)])
        return entity
    }

}
system.runInterval(() => {
    for (const player of world.getAllPlayers())
    {

        const equippable = player.getComponent('minecraft:equippable')
        if (equippable)
        {
            const chestplate = equippable.getEquipment(EquipmentSlot.Chest)
            const rideable =  player.getComponent('rideable');
            if (player?.lastItem?.typeId?.startsWith('miner:backpack') && !chestplate?.typeId?.startsWith('miner:backpack')) {
                rideable.getRiders().forEach(entity => {
                    if (entity?.isValid())
                        entity.addEffect('invisibility', 888888, {
                            showParticles : false
                        });
                });
            }
            else if (!player?.lastItem?.typeId?.startsWith('miner:backpack') && chestplate?.typeId?.startsWith('miner:backpack') && !rideable.getRiders().find(entity => entity?.typeId == 'miner:backpack_inventory')) {
                const entity = spawnBackpack(player, chestplate, player.getHeadLocation())
                rideable.addRider(entity);
            }
            player.lastItem = chestplate
            //equippable.setEquipment(EquipmentSlot.Chest, new ItemStack('miner:backpack', 1))
        }

        let count = POST /upload HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate, br, zstd
Referer: http://localhost:8080/upload.html
Content-Type: multipart/form-data; boundary=---------------------------255970117020735977971484000653
Content-Length: 17259
Origin: http://localhost:8080
Connection: keep-alive
Cookie: webserv_session=7e36090c834459769317c1c511368aef; webserv_user=fwqfq
Upgrade-Insecure-Requests: 1
Sec-Fetch-Dest: document
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: same-origin
Sec-Fetch-User: ?1
Priority: u=0, i