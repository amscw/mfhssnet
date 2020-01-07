#include "mfhssnet.h"
#include "mfhssfs.h"

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
// файлы регистров
struct reg_attribute {
	struct attribute default_attribute;
	u32 address;
	u32 value;
	char name[32];	// TODO: хорошо бы динамически, чтобы не расходовать память зря
} __attribute__((__packed__));

// каталоги регистров
struct reg_group {
	struct kobject kobj;
	struct mfhss_priv_ *priv;	// set by probe() function in run-time
};

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
// hardcoded registers
#define REG_DMA_CR_NAME			"cr"
#define REG_DMA_CR_ADDRESS		0x0004
#define REG_DMA_SR_NAME			"sr"
#define REG_DMA_SR_ADDRESS		0x0008
#define REG_DMA_IR_NAME			"ir"
#define REG_DMA_IR_ADDRESS		0x000C
#define REG_DMA_SA_NAME			"sa"
#define REG_DMA_SA_ADDRESS		0x0010
#define REG_DMA_DA_NAME			"da"
#define REG_DMA_DA_ADDRESS		0x0014
#define REG_DMA_SL_NAME			"sl"
#define REG_DMA_SL_ADDRESS		0x0018
#define REG_DMA_DL_NAME			"dl"
#define REG_DMA_DL_ADDRESS		0x001C
#define REG_MLIP_SR_NAME		"sr"
#define REG_MLIP_SR_ADDRESS		0x0020
#define REG_MLIP_IR_NAME		"ir"
#define REG_MLIP_IR_ADDRESS		0x0024
#define REG_MLIP_RST_NAME		"rst"
#define REG_MLIP_RST_ADDRESS	0x0028
#define REG_MLIP_CE_NAME		"ce"
#define REG_MLIP_CE_ADDRESS		0x002C

// имя переменной-регистра (атрибута)
#define REG(group, reg) reg_##group##_##reg

// имя переменной-группы
#define GROUP_TYPE(group) group##_type

// создает регистр (атрибут sysfs)
#define MAKE_REG(group, reg) \
	static struct reg_attribute REG(group, reg) = {\
		{\
			.name = REG_##group##_##reg##_NAME,\
			.mode = S_IRUGO | S_IWUSR\
		}, REG_##group##_##reg##_ADDRESS, 0\
}

// создает объект с операциями над регистром (sysfs_ops)
#define MAKE_GROUP_OPS(group) \
	static void release_##group(struct kobject *kobj)\
	{\
		struct reg_group *g = container_of(kobj, struct reg_group, kobj);\
		PDEBUG("destroying object: %s\n", kobj->name);\
		kfree(g);\
	}\
	\
	static ssize_t sysfs_show_##group(struct kobject *kobj, struct attribute *attr, char *buf)\
	{\
		unsigned long flags = 0;\
		struct reg_group *g = container_of(kobj, struct reg_group, kobj);\
		struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);\
		spin_lock_irqsave(&g->priv->lock, flags);\
		a->value = ioread32((void __iomem*)(g->priv->io_base + a->address));\
		spin_unlock_irqrestore(&g->priv->lock, flags);\
		PDEBUG("read from %s@0x%X = 0x%X\n", attr->name, a->address, a->value);\
		return scnprintf(buf, PAGE_SIZE, "%d\n", a->value);\
	}\
	\
	static ssize_t sysfs_store_##group(struct kobject *kobj, struct attribute* attr, const char *buf, size_t len)\
	{\
		unsigned long flags = 0;\
		struct reg_group *g = container_of(kobj, struct reg_group, kobj);\
		struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);\
		sscanf(buf, "%d", &a->value);\
		spin_lock_irqsave(&g->priv->lock, flags);\
		iowrite32(a->value, (void __iomem*)(g->priv->io_base + a->address));\
		spin_unlock_irqrestore(&g->priv->lock, flags);\
		PDEBUG("write 0x%X to %s@0x%X\n", a->value, a->default_attribute.name, a->address);\
		return len;\
	}\
	\
	static struct sysfs_ops group##_ops = {\
		.show = sysfs_show_##group,\
		.store = sysfs_store_##group,\
	};\

// создает тип регистра (тип kobject)
#define MAKE_GROUP_TYPE(group) \
	MAKE_GROUP_OPS(group);\
	struct kobj_type GROUP_TYPE(group) = {\
		.sysfs_ops = &group##_ops,\
		.default_attrs = group##_attributes,\
		.release = release_##group\
	};

//-------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------
// hardcoded DMA registers
MAKE_REG(DMA, CR);
MAKE_REG(DMA, SR);
MAKE_REG(DMA, IR);
MAKE_REG(DMA, SA);
MAKE_REG(DMA, DA);
MAKE_REG(DMA, SL);
MAKE_REG(DMA, DL);
struct attribute *DMA_attributes[] = {
	&REG(DMA, CR).default_attribute,
	&REG(DMA, SR).default_attribute,
	&REG(DMA, IR).default_attribute,
	&REG(DMA, SA).default_attribute,
	&REG(DMA, DA).default_attribute,
	&REG(DMA, SL).default_attribute,
	&REG(DMA, DL).default_attribute,
	NULL
};
MAKE_GROUP_TYPE(DMA);

// hardcoded MLIP registers
MAKE_REG(MLIP, SR);
MAKE_REG(MLIP, IR);
MAKE_REG(MLIP, RST);
MAKE_REG(MLIP, CE);
struct attribute *MLIP_attributes[] = {
	&REG(MLIP, SR).default_attribute,
	&REG(MLIP, IR).default_attribute,
	&REG(MLIP, RST).default_attribute,
	&REG(MLIP, CE).default_attribute,
	NULL
};
MAKE_GROUP_TYPE(MLIP);
